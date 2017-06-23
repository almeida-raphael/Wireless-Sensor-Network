#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
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

#define UDP_PORT 1234
#define SERVICE_ID 190

#define SEND_INTERVAL   (10 * CLOCK_SECOND)
#define SEND_TIME   (random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection connection;

struct msgStruct {
  uint8_t sender[16];
  uint16_t light1;
  uint16_t light2;
  uint16_t temperature;
  uint16_t humidity;
  uint32_t energy_lpm;
  uint32_t energy_cpu;
  uint32_t energy_rx;
  uint32_t energy_tx;
  uint32_t energy_rled;
  int data_used;
  uint32_t timestamp;
};

static struct msgStruct devices[4];
static int devices_len = 0;

struct msgStruct createData(const uip_ipaddr_t *sender_addr, uint32_t *data){
  struct msgStruct device_data;
  device_data.sender[0] = sender_addr->u8[0];
  device_data.sender[1] = sender_addr->u8[1];
  device_data.sender[2] = sender_addr->u8[2];
  device_data.sender[3] = sender_addr->u8[3];
  device_data.sender[4] = sender_addr->u8[4];
  device_data.sender[5] = sender_addr->u8[5];
  device_data.sender[6] = sender_addr->u8[6];
  device_data.sender[7] = sender_addr->u8[7];
  device_data.sender[8] = sender_addr->u8[8];
  device_data.sender[9] = sender_addr->u8[9];
  device_data.sender[10] = sender_addr->u8[10];
  device_data.sender[11] = sender_addr->u8[11];
  device_data.sender[12] = sender_addr->u8[12];
  device_data.sender[13] = sender_addr->u8[13];
  device_data.sender[14] = sender_addr->u8[14];
  device_data.sender[15] = sender_addr->u8[15];
  device_data.light1 = data[0];
  device_data.light2 = data[1];
  device_data.temperature = data[2];
  device_data.humidity = data[3];
  device_data.energy_lpm = data[4];
  device_data.energy_cpu = data[5];
  device_data.energy_rx = data[6];
  device_data.energy_tx = data[7];
  device_data.energy_rled = data[8];
  device_data.data_used = 0;
  return device_data;
}

int checkSender(uint8_t sender1[16], uint8_t sender2[16]){
   int i;
   for(i = 0; i<16; i++){
     if(sender1[i] != sender2[i]){
      return 0;
     }
   }
   return 1;
}

int updateDeviceInfo(const uip_ipaddr_t *sender_addr, uint32_t *data){
  struct msgStruct device_data = createData(sender_addr, data);
  int i;
  for(i = 0; i < devices_len; i++){
    if(checkSender(device_data.sender, devices[i].sender) == 1){
      devices[i] = device_data;
      return i;
    }
  }

  if(devices_len<4){
    devices[devices_len] = device_data;
    devices_len++;
    return devices_len-1;
  }
}


/*---------------------------------------------------------------------------*/
struct msgStruct combineData(){
  struct msgStruct msg;

  msg.light1 = devices[0].light1;
  msg.light2 = devices[0].light2;
  msg.temperature = devices[0].temperature;
  msg.humidity = devices[0].humidity;
  msg.energy_lpm = devices[0].energy_lpm;
  msg.energy_cpu = devices[0].energy_cpu;
  msg.energy_rx = devices[0].energy_rx;
  msg.energy_tx = devices[0].energy_tx;
  msg.energy_rled = devices[0].energy_rled;

  int i;
  for(i = 1; i<4; i++){
    msg.light1 += devices[i].light1;
    msg.light2 += devices[i].light2;
    msg.temperature += devices[i].temperature;
    msg.humidity += devices[i].humidity;
    msg.energy_lpm += devices[i].energy_lpm;
    msg.energy_cpu += devices[i].energy_cpu;
    msg.energy_rx += devices[i].energy_rx;
    msg.energy_tx += devices[i].energy_tx;
    msg.energy_rled += devices[i].energy_rled;
  }

  msg.light1 /= 4;
  msg.light2 /= 4;
  msg.temperature /= 4;
  msg.humidity /= 4;
  msg.energy_lpm /= 4;
  msg.energy_cpu /= 4;
  msg.energy_rx /= 4;
  msg.energy_tx /= 4;
  msg.energy_rled /= 4;

//  time_t t;
//  time(&t);
  msg.timestamp = clock_seconds();

  return msg;
}

void saveCombinedData(struct msgStruct data){
//  FILE *pFile = fopen("dataReport.txt", "a");
//
//  if(pFile==NULL) {
//    perror("Error opening file.");
//  }
//  else {
//    fprintf(pFile, "\n------------------------------------\n"); 
//    fprintf(pFile, "Timestamp: %d-%d-%d %d:%d:%d\n", data.timestamp.tm_year + 1900, data.timestamp.tm_mon + 1, data.timestamp.tm_mday, data.timestamp.tm_hour, data.timestamp.tm_min, data.timestamp.tm_sec); 
//    fprintf(pFile, "Combination Method: Average\n"); 
//    fprintf(pFile, "Light 1 Sensor: %u\n", data.light1); 
//    fprintf(pFile, "Light 2 Sensor: %u\n", data.light2); 
//    fprintf(pFile, "Temperature Sensor: %u\n", data.temperature); 
//    fprintf(pFile, "Humidity Sensor: %u\n", data.humidity); 
//    fprintf(pFile, "Energy LPM: %lu\n", data.energy_lpm); 
//    fprintf(pFile, "Energy CPU: %lu\n", data.energy_cpu);  
//    fprintf(pFile, "Energy RX: %lu\n", data.energy_rx); 
//    fprintf(pFile, "Energy TX: %lu\n", data.energy_tx); 
//    fprintf(pFile, "Energy Red Led: %lu\n", data.energy_rled); 
//    fprintf(pFile, "------------------------------------\n"); 
//  }
//  fclose(pFile);    

  printf("\n------------------------------------\n"); 
  printf("Time Since Start: %lu\n", data.timestamp); 
  printf("Combination Method: Average\n"); 
  printf("Light 1 Sensor: %u\n", data.light1); 
  printf("Light 2 Sensor: %u\n", data.light2); 
  printf("Temperature Sensor: %u\n", data.temperature); 
  printf("Humidity Sensor: %u\n", data.humidity); 
  printf("Energy LPM: %lu\n", data.energy_lpm); 
  printf("Energy CPU: %lu\n", data.energy_cpu);  
  printf("Energy RX: %lu\n", data.energy_rx); 
  printf("Energy TX: %lu\n", data.energy_tx); 
  printf("Energy Red Led: %lu\n", data.energy_rled); 
  printf("------------------------------------\n"); 
} 

/*---------------------------------------------------------------------------*/
PROCESS(receiver_process, "Receiver process");
AUTOSTART_PROCESSES(&receiver_process);
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

  printf("RECEIVER: Data received from ");
  uip_debug_ipaddr_print(sender_addr);

  printf(" on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);

  
  int device_id = updateDeviceInfo(sender_addr, data);

//  printf("\n---------------------------------------\n");
//  printf("Device ID: %d\n", device_id);
//  printf("Sender: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n", 
//    devices[device_id].sender[0], devices[device_id].sender[1], devices[device_id].sender[2], devices[device_id].sender[3],
//    devices[device_id].sender[4], devices[device_id].sender[5], devices[device_id].sender[6], devices[device_id].sender[7],
//    devices[device_id].sender[8], devices[device_id].sender[9], devices[device_id].sender[10], devices[device_id].sender[11],
//    devices[device_id].sender[12], devices[device_id].sender[13], devices[device_id].sender[14], devices[device_id].sender[15]
//  );

//  printf("Light1: %u\n", devices[device_id].light1);
//  printf("Light2: %u\n", devices[device_id].light2);
//  printf("Temperature: %u\n", devices[device_id].temperature);
//  printf("Humidity: %u\n", devices[device_id].humidity);
//  printf("Energy LPM: %lu\n", devices[device_id].energy_lpm);
//  printf("Energy CPU: %lu\n", devices[device_id].energy_cpu);
//  printf("Energy RX: %lu\n", devices[device_id].energy_rx);
//  printf("Energy TX: %lu\n", devices[device_id].energy_tx);
//  printf("Energy Red LED: %lu\n", devices[device_id].energy_rled);
//  printf("---------------------------------------\n");

  if(devices_len >= 4){
    int i;
    int data_ready = 1;
    for(i = 0; i<4; i++){
      if(devices[i].data_used == 1){
        data_ready = 0;
      }
    }
    if(data_ready == 1){
      for(i = 0; i<4; i++){
        devices[i].data_used = 1;
      }
      saveCombinedData(combineData());
    }
  }
  
  leds_off(LEDS_GREEN);   
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
/*---------------------------------------------------------------------------*/
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

  while(1) {
    PROCESS_WAIT_EVENT();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
