#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"

#include "simple-udp.h"
#include "servreg-hack.h"

#include "net/rpl/rpl.h"

#include "dev/leds.h"
#include "dev/button-sensor.h"

#define UDP_PORT 1234
#define SERVICE_ID 190

#define SEND_INTERVAL   (10 * CLOCK_SECOND)
#define SEND_TIME   (random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection connection;

#define DEC1 10
#define DEC2 100
#define DEC3 1000
#define DEC4 10000
#define DEC5 100000
#define DEC6 1000000

typedef struct {
  int light1;
  int light2;
  int16_t temperature;
  int16_t humidity;
  uint32_t energy_lpm;
  uint32_t energy_cpu;
  uint32_t energy_rx;
  uint32_t energy_tx;
} dataTypes;

uip_ipaddr_t senderMapping[4];
int lengthSenderMapping = 0;  

dataTypes avalibleData[4];
short dataUsed[4] = {1,1,1,1};

double tSum = 0;
dataTypes dSum;
dataTypes tdCovSum;
double tVarSum = 0;

dataTypes dSumKG;
dataTypes tdCovSumKG;
double tVarSumKG = 0;

dataTypes a;
dataTypes b;
dataTypes aKG;
dataTypes bKG;

uint32_t dataSize = 0;

float decay = 0.5;
float decayCorrected = 0.4;

#define BLUE 1
#define RED 2
#define GREEN 4

/*---------------------------------------------------------------------------*/
void putDouble(double f, int p){
    printf("%d/%d\n", (int)(f*p), (int)(p));
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
dataTypes product(dataTypes a, dataTypes b){
  dataTypes temp;
  temp.light1 = a.light1 * b.light1;
  temp.light2 = a.light2 * b.light2;
  temp.temperature = a.temperature * b.temperature;
  temp.humidity = a.humidity * b.humidity;
  temp.energy_lpm = a.energy_lpm * b.energy_lpm;
  temp.energy_cpu = a.energy_cpu * b.energy_cpu;
  temp.energy_rx = a.energy_rx * b.energy_rx;
  temp.energy_tx = a.energy_tx * b.energy_tx;
  return temp;
}

dataTypes quocient(dataTypes a, dataTypes b){
  dataTypes temp;
  temp.light1 = a.light1 / b.light1;
  temp.light2 = a.light2 / b.light2;
  temp.temperature = a.temperature / b.temperature;
  temp.humidity = a.humidity / b.humidity;
  temp.energy_lpm = a.energy_lpm / b.energy_lpm;
  temp.energy_cpu = a.energy_cpu / b.energy_cpu;
  temp.energy_rx = a.energy_rx / b.energy_rx;
  temp.energy_tx = a.energy_tx / b.energy_tx;
  return temp;
}

dataTypes quocientConstant(dataTypes a, double b){
  dataTypes temp;
  temp.light1 = a.light1 / b;
  temp.light2 = a.light2 / b;
  temp.temperature = a.temperature / b;
  temp.humidity = a.humidity / b;
  temp.energy_lpm = a.energy_lpm / b;
  temp.energy_cpu = a.energy_cpu / b;
  temp.energy_rx = a.energy_rx / b;
  temp.energy_tx = a.energy_tx / b;
  return temp;
}

dataTypes productConstant(dataTypes a, double b){
  dataTypes temp;
  temp.light1 = a.light1 * b;
  temp.light2 = a.light2 * b;
  temp.temperature = a.temperature * b;
  temp.humidity = a.humidity * b;
  temp.energy_lpm = a.energy_lpm * b;
  temp.energy_cpu = a.energy_cpu * b;
  temp.energy_rx = a.energy_rx * b;
  temp.energy_tx = a.energy_tx * b;
  return temp;
}

dataTypes sum(dataTypes a, dataTypes b){
  dataTypes temp;
  temp.light1 = a.light1 + b.light1;
  temp.light2 = a.light2 + b.light2;
  temp.temperature = a.temperature + b.temperature;
  temp.humidity = a.humidity + b.humidity;
  temp.energy_lpm = a.energy_lpm + b.energy_lpm;
  temp.energy_cpu = a.energy_cpu + b.energy_cpu;
  temp.energy_rx = a.energy_rx + b.energy_rx;
  temp.energy_tx = a.energy_tx + b.energy_tx;
  return temp;
}

dataTypes subtract(dataTypes a, dataTypes b){
  dataTypes temp;
  temp.light1 = a.light1 - b.light1;
  temp.light2 = a.light2 - b.light2;
  temp.temperature = a.temperature - b.temperature;
  temp.humidity = a.humidity - b.humidity;
  temp.energy_lpm = a.energy_lpm - b.energy_lpm;
  temp.energy_cpu = a.energy_cpu - b.energy_cpu;
  temp.energy_rx = a.energy_rx - b.energy_rx;
  temp.energy_tx = a.energy_tx - b.energy_tx;
  return temp;
}

dataTypes subtractConstant(dataTypes a, double b){
  dataTypes temp;
  temp.light1 = a.light1 - b;
  temp.light2 = a.light2 - b;
  temp.temperature = a.temperature - b;
  temp.humidity = a.humidity - b;
  temp.energy_lpm = a.energy_lpm - b;
  temp.energy_cpu = a.energy_cpu - b;
  temp.energy_rx = a.energy_rx - b;
  temp.energy_tx = a.energy_tx - b;
  return temp;
}

void printDataType(dataTypes a){
  printf("Light 1: %d - ", a.light1);
  printf("Light 2: %d - ", a.light2);
  printf("Temperature: %d - ", a.temperature);
  printf("Humidity: %d - ", a.humidity);
  printf("Energy LPM: %lu - ", a.energy_lpm);
  printf("Energy CPU: %lu - ", a.energy_cpu); 
  printf("Energy RX: %lu - ", a.energy_rx);
  printf("Energy TX: %lu\n", a.energy_tx);
}

void printIPV6(uip_ipaddr_t IPV6){
  printf("%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u\n", 
    IPV6.u8[0],
    IPV6.u8[1],
    IPV6.u8[2],
    IPV6.u8[3],
    IPV6.u8[4],
    IPV6.u8[5],
    IPV6.u8[6],
    IPV6.u8[7],
    IPV6.u8[8],
    IPV6.u8[9],
    IPV6.u8[10],
    IPV6.u8[11],
    IPV6.u8[12],
    IPV6.u8[13],
    IPV6.u8[14],
    IPV6.u8[15]
  ); 
}

void printDebugStep(){
  printf("################# SENDER DEBUG STEP #################\n");
  printf("senderMapping:\n");
  printf("\t 0: ");
  printIPV6(senderMapping[0]);
  printf("\t 1: ");
  printIPV6(senderMapping[1]);
  printf("\t 2: ");
  printIPV6(senderMapping[2]);
  printf("\t 3: ");
  printIPV6(senderMapping[3]);
  printf("\n");
  printf("lengthSenderMapping: %d\n", lengthSenderMapping);
  printf("\n");
  printf("avalibleData:\n");
  printf("\t 0: ");
  printDataType(avalibleData[0]);
  printf("\t 1: ");
  printDataType(avalibleData[1]);
  printf("\t 2: ");
  printDataType(avalibleData[2]);
  printf("\t 3: ");
  printDataType(avalibleData[3]);
  printf("\n");
  printf("dataUsed:\n");
  printf("\t0: %d - 1: %d - 2: %d - 3: %d\n", dataUsed[0], dataUsed[1], dataUsed[2], dataUsed[3]);
  printf("\n");
  printf("tSum: ");
  putDouble(tSum, DEC4);
  printf("\n");
  printf("dSum: \n");
  printDataType(dSum);
  printf("\n");
  printf("tdCovSum: \n");
  printDataType(tdCovSum);
  printf("\n");
  printf("tVarSum: ");
  putDouble(tVarSum, DEC4);
  printf("\n");
  printf("dSumKG: \n");
  printDataType(dSumKG);
  printf("\n");
  printf("tdCovSumKG: \n");
  printDataType(tdCovSumKG);
  printf("\n");
  printf("tVarSumKG: ");
  putDouble(tVarSumKG, DEC4);
  printf("\n");
  printf("a: \n");
  printDataType(a);
  printf("\n");
  printf("b: \n");
  printDataType(b);
  printf("\n");
  printf("aKG: \n");
  printDataType(aKG);
  printf("\n");
  printf("bKG: \n");
  printDataType(bKG);
  printf("\n");
  printf("dataSize: %lu\n", dataSize);
  printf("\n");
  printf("decay: ");
  putDouble(decay, DEC4);    
  printf("\n");
  printf("decayCorrected: ");
  putDouble(decayCorrected, DEC4);
  printf("\n");
  printf("######################################################\n");
}

dataTypes sumConstant(dataTypes a, double b){
  dataTypes temp;
  temp.light1 = a.light1 + b;
  temp.light2 = a.light2 + b;
  temp.temperature = a.temperature + b;
  temp.humidity = a.humidity + b;
  temp.energy_lpm = a.energy_lpm + b;
  temp.energy_cpu = a.energy_cpu + b;
  temp.energy_rx = a.energy_rx + b;
  temp.energy_tx = a.energy_tx + b;
  return temp;
}
/*---------------------------------------------------------------------------*/

int compareIPV6(uip_ipaddr_t ip1, uip_ipaddr_t ip2){
  int i;
  for(i = 0; i < 16; i++){
    if(ip1.u8[i] != ip2.u8[i]){
      return 0;
    }
  }
  return 1;
}

/*---------------------------------------------------------------------------*/
static uip_ipaddr_t *
set_global_address(void)
{
  static uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  printf("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }

  return &ipaddr;
}

static void
create_rpl_dag(uip_ipaddr_t *ipaddr)
{
  struct uip_ds6_addr *root_if;

  root_if = uip_ds6_addr_lookup(ipaddr);
  if(root_if != NULL) {
    rpl_dag_t *dag;
    uip_ipaddr_t prefix;
    
    rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
    dag = rpl_get_any_dag();
    uip_ip6addr(&prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &prefix, 64);
    printf("created a new RPL dag\n");
  } else {
    leds_on(RED);   
    printf("failed to create a new RPL DAG\n");
  }
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS(receiver_process, "Receiver process");
AUTOSTART_PROCESSES(&receiver_process);
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
dataTypes calcCov(uint32_t x, dataTypes y, double xAvg, dataTypes yAvg, dataTypes covSum, int dataSize){
  return 
    quocientConstant(
      sum(
        productConstant(
          subtract(
            y,
            yAvg 
          ),
          (x - xAvg)
        ),
        covSum
      ),
      dataSize - 1 + 1e-10
    )
  ;
}

dataTypes updateCovSum(uint32_t x, double xAvg, dataTypes y, dataTypes yAvg, dataTypes xyCovSum, double decay){
  return
    sum(
      productConstant(
        subtract(
          y,
          yAvg
        ),
        (x - xAvg)
      ),
      productConstant(
        xyCovSum,
        decay
      )
    )
  ; 
}

dataTypes angularCoef(uint32_t x, dataTypes y, double xAvg, dataTypes yAvg, dataTypes xyCovSum, double xxCovSum, uint32_t dataSize){
  return quocientConstant(calcCov(x, y, xAvg, yAvg, xyCovSum, dataSize), ((xxCovSum + ((x-xAvg)*(x-xAvg))) / (dataSize - 1 + 1e-10)));
}

dataTypes linearCoef(dataTypes ang, double xAvg, dataTypes yAvg){
  return subtract(yAvg, productConstant(ang, xAvg));
}

dataTypes kalmanCoef(dataTypes estimate, dataTypes measurement){
  return quocient(estimate, sum(estimate, measurement));
}

dataTypes kalmanFilter(dataTypes estimate, dataTypes measurement){
  return sum(estimate, product(kalmanCoef(estimate, measurement), (subtract(measurement, estimate))));
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
dataTypes predict(uint32_t x, dataTypes a, dataTypes b){
  return sum(productConstant(a, x), b);
}

void update(dataTypes d){
  printDebugStep();

  dataSize++;
  uint32_t t = clock_seconds();

  tSum += t;
  dSum = sum(dSum, d);

  double tAvg = tSum / dataSize;
  dataTypes dAvg = quocientConstant(dSum, dataSize);
  
  a = angularCoef(t, d, tAvg, dAvg, tdCovSum, tVarSum, dataSize);
  b = linearCoef(a, tAvg, dAvg);

  dataTypes predict = sum(productConstant(a, t), b);
  dataTypes corrected = kalmanFilter(predict, d);

  dSumKG = sum(dSumKG, corrected);

  dataTypes dAvgKG = quocientConstant(dSumKG, dataSize);

  aKG = angularCoef(t, corrected, tAvg, dAvgKG, tdCovSumKG, tVarSum, dataSize);
  bKG = linearCoef(aKG, tAvg, dAvgKG);

  tdCovSum = updateCovSum(t, tAvg, d, dAvg, tdCovSum, decay);
  tdCovSumKG = updateCovSum(t, tAvg, corrected, dAvgKG, tdCovSumKG, decayCorrected);
  tVarSum =  (decay * tVarSum) + ((t - tAvg)*(t - tAvg));

  printf("Model Updated at %lu\n", clock_seconds());
}

void insertDataIntoDataSlot(int index, uint32_t *data){
  avalibleData[index].light1 = data[0];
  avalibleData[index].light2 = data[1];
  avalibleData[index].temperature = data[2];
  avalibleData[index].humidity = data[3];
  avalibleData[index].energy_lpm = data[4];
  avalibleData[index].energy_cpu = data[5];
  avalibleData[index].energy_rx = data[6];
  avalibleData[index].energy_tx = data[7];
  dataUsed[index] = 0;
}

int isDataReady(){
  int i;
  for(i = 0; i < 4; i++){
    if(dataUsed[i] == 1){
      return 0;
    }
  }
  return 1;
}

void resetDataReady(){
  int i;
  for(i = 0; i < 4; i++){
    dataUsed[i] = 1;
  }
}

dataTypes initDataType(){
  dataTypes temp;
  temp = productConstant(temp, 0);
  return temp;
}

dataTypes combineData(){
  int i;
  dataTypes returnData = initDataType();
  for(i = 0; i < 4; i++){
    returnData = sum(returnData, avalibleData[i]);
  }
  returnData = quocientConstant(returnData, 4);
  return returnData;
}

int getSenderIndex(uip_ipaddr_t sender_addr){
  int i;
  for(i = 0; i < 4; i++){
    if(compareIPV6(sender_addr, senderMapping[i]) == 1){
      return i;
    }
  }
  if(lengthSenderMapping >= 4){
    return -1;
  }
  senderMapping[lengthSenderMapping] = sender_addr;
  lengthSenderMapping++;
  return lengthSenderMapping-1;
}

void processIncomingData(uip_ipaddr_t *sender_addr, int32_t *data){
  int index = getSenderIndex(*sender_addr);
  if(index != -1){
    insertDataIntoDataSlot(index, data);
    printDataType(avalibleData[index]);
    if(isDataReady() == 1){
      resetDataReady();
      dataTypes combinedData = combineData();
      update(combinedData);
    }
  }
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const int32_t *data,
         uint16_t datalen)
{
  leds_on(GREEN);   
  leds_off(RED);   

  printf("RECEIVER: Data received from ");
  uip_debug_ipaddr_print(sender_addr);

  printf(" on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);

  processIncomingData(sender_addr, data);
  
  leds_off(GREEN);   
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
uip_ipaddr_t* getSenderMapping(){
  return senderMapping;
}

int getLengthSenderMapping(){
  return lengthSenderMapping;
}

dataTypes* getAvaliableData(){
  return avalibleData;
}

short* getDataUsed(){
  return dataUsed;
}

double getTSum(){
  return tSum;
}

dataTypes getDSum(){
  return dSum;
}

dataTypes getTdCovSum(){
  return tdCovSum;
}

double getTVarSum(){
  return tVarSum;
}

dataTypes getDSumKG(){
  return dSumKG;
}

dataTypes getTdCovSumKG(){
  return tdCovSumKG;
}

double getTVarSumKG(){
  return tVarSumKG;
}

dataTypes getA(){
  return a;
}

dataTypes getB(){
  return b;
}

dataTypes getAKG(){
  return aKG;
}

dataTypes getBKG(){
  return bKG;
}

uint32_t getDataSize(){
  return dataSize;
}

float getDecay(){
  return decay;
}

float getDecayCorrected(){
  return decayCorrected;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(receiver_process, ev, data)
{
  uip_ipaddr_t *ipaddr;
  
  dataTypes dSum = initDataType();
  dataTypes tdCovSum = initDataType();
  dataTypes dSumKG = initDataType();
  dataTypes tdCovSumKG = initDataType();
  dataTypes a = initDataType();
  dataTypes b = initDataType();
  dataTypes aKG = initDataType();
  dataTypes bKG = initDataType();

  PROCESS_BEGIN();

  leds_on(BLUE);

  servreg_hack_init();
  ipaddr = set_global_address();
  create_rpl_dag(ipaddr);
  servreg_hack_register(SERVICE_ID, ipaddr);
  simple_udp_register(&connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  SENSORS_ACTIVATE(button_sensor);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    leds_on(GREEN);  
    leds_off(RED);   
    printf("Prediction: \n");
    printDataType(predict(clock_seconds(), getAKG(), getBKG()));
    leds_off(GREEN);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/