#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef struct {
  double light1;
  double light2;
  double temperature;
  double humidity;
  double energy_lpm;
  double energy_cpu;
  double energy_rx;
  double energy_tx;
  double energy_rled;
} dataTypes;

uip_ipaddr_t *senderMapping[4];
int lengthSenderMapping = 0;

dataTypes avalibleData[4];
short dataUsed[4] = {1,1,1,1}

dataTypes tSum;
dataTypes dSum;
dataTypes tdComSum;
dataTypes tVarSum;

dataTypes dSumKG;
dataTypes tdCovSumKG;
dataTypes tVarSumKG;

dataTypes a;
dataTypes b;
dataTypes aKG;
dataTypes bKG;

dataTypes dataSize;

float decay = 0.5;
float decayCorrected = 0.4;

uint32_t lastDataAvaliableTime = 0;

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
    PRINTF("created a new RPL dag\n");
  } else {
    leds_toggle(LEDS_RED);   
    PRINTF("failed to create a new RPL DAG\n");
    leds_toggle(LEDS_RED);   
  }
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
PROCESS(receiver_process, "Receiver process");
AUTOSTART_PROCESSES(&receiver_process);
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
int getSenderIndex(uip_ipaddr_t *sender_addr){
  for(i = 0; i < 4; ++i){
    if(sender_addr == senderMapping[i])
      return i;
  }
  if(lengthSenderMapping >= 4){
    return -1;
  }
  senderMapping[lengthSenderMapping] = sender_addr;
  sender_addr++;
  return sender_addr-1;
}

double calcCov(double x, double y, double xAvg, double yAvg, covSum, int dataSize){
  return (covSum + (x - xAvg)(y - yAvg)) / (dataSize - 1 + 1e-10);
}

double angularCoef(double x, double y, double xAvg, double yAvg, double xyCovSum, double xxCovSum, int dataSize){
  return calcCov(x, y, xAvg, yAvg, xyCovSum, dataSize) / (calcCov(x, x, xAvg, xAvg, xxCovSum, dataSize) + 1e-10);
}

double linearCoef(double ang, double xAvg, double yAvg, int dataSize){
  return yAvg - ang*xAvg;
}

double kalmanCoef(double estimate, double measurement){
  return estimate / (estimate + measurement);
}

double kalmanFilter(double estimate, double measurement){
  return estimate + kalmanCoef(estimate, measurement) * (measurement - estimate);
}

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
  temp.energy_rled = a.energy_rled * b.energy_rled;
  return temp
}

dataTypes productDouble(dataTypes a, double b){
  dataTypes temp;
  temp.light1 = a.light1 * b;
  temp.light2 = a.light2 * b;
  temp.temperature = a.temperature * b;
  temp.humidity = a.humidity * b;
  temp.energy_lpm = a.energy_lpm * b;
  temp.energy_cpu = a.energy_cpu * b;
  temp.energy_rx = a.energy_rx * b;
  temp.energy_tx = a.energy_tx * b;
  temp.energy_rled = a.energy_rled * b;
  return temp
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
  temp.energy_rled = a.energy_rled + b.energy_rled;
  return temp
}

dataTypes sumDouble(dataTypes a, double b){
  dataTypes temp;
  temp.light1 = a.light1 + b;
  temp.light2 = a.light2 + b;
  temp.temperature = a.temperature + b;
  temp.humidity = a.humidity + b;
  temp.energy_lpm = a.energy_lpm + b;
  temp.energy_cpu = a.energy_cpu + b;
  temp.energy_rx = a.energy_rx + b;
  temp.energy_tx = a.energy_tx + b;
  temp.energy_rled = a.energy_rled + b;
  return temp
}

dataTypes predict(dataTypes x, dataTypes a, dataTypes b){
  return sum(productDouble(a, x), b);
}

void update(dataTypes processedData){
  dataSize++;
  //TODO: Continuar daqui
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
  avalibleData[index].energy_rled = data[8];
  dataUsed[index] = 0;
}

int isDataReady(){
  int flag = 0
  int i;
  for(i = 0; i < 4; i++){
    if(dataUsed[i] == 1){
      return 0;
    }
  }
  return 1;
}

dataTypes combineData(){
  int i;
  dataTypes returnData;
  for(i = 0; i < 4; i++){
    returnData.light1 += avalibleData[i].light1;
    returnData.light2 += avalibleData[i].light2;
    returnData.temperature += avalibleData[i].temperature;
    returnData.humidity += avalibleData[i].humidity;
    returnData.energy_lpm += avalibleData[i].energy_lpm;
    returnData.energy_cpu += avalibleData[i].energy_cpu;
    returnData.energy_rx += avalibleData[i].energy_rx;
    returnData.energy_tx += avalibleData[i].energy_tx;
    returnData.energy_rled += avalibleData[i].energy_rled;
  }
  returnData.light1 /= double(4);
  returnData.light2 /= double(4);
  returnData.temperature /= double(4);
  returnData.humidity /= double(4);
  returnData.energy_lpm /= double(4);
  returnData.energy_cpu /= double(4);
  returnData.energy_rx /= double(4);
  returnData.energy_tx /= double(4);
  returnData.energy_rled /= double(4); 
  return returnData;
}

void processIncomingData(uip_ipaddr_t *sender_addr, uint32_t *data){
  int index = getSenderIndex(sender_addr);
  insertDataIntoDataSlot(index, data);
  if(isDataReady() == 1){
    dataTypes processedData = combineData();
    update(processedData);
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
         const uint32_t *data,
         uint16_t datalen)
{
  leds_on(LEDS_GREEN);   
  leds_off(LEDS_RED);   

  printf("RECEIVER: Data received from ");
  uip_debug_ipaddr_print(sender_addr);

  printf(" on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);

  processIncomingData(sender_addr, data)
  
  leds_off(LEDS_GREEN);   
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(receiver_process, ev, data)
{
  uip_ipaddr_t *ipaddr;

  PROCESS_BEGIN();

  leds_on(LEDS_BLUE);

  servreg_hack_init();
  ipaddr = set_global_address();
  create_rpl_dag(ipaddr);
  servreg_hack_register(SERVICE_ID, ipaddr);
  simple_udp_register(&connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  SENSORS_ACTIVATE(button_sensor);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    printf("RECEIVER: Request Data\n");

    leds_on(LEDS_GREEN);  

    leds_off(LEDS_GREEN);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
