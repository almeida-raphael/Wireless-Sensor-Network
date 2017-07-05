/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"

#include "sys/node-id.h"

#include "simple-udp.h"
#include "servreg-hack.h"

#include <stdio.h>
#include <string.h>

#include "dev/leds.h"
#include "dev/light-sensor.h"
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
#define UDP_PORT 1234
#define SERVICE_ID 190

#define SEND_INTERVAL   (30 * CLOCK_SECOND)
#define SEND_TIME   (random_rand() % (SEND_INTERVAL))
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static struct simple_udp_connection connection;
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
#define BLUE 1
#define RED 2
#define GREEN 4
/*---------------------------------------------------------------------------*/

uint32_t old_e_cpu;
uint32_t old_e_rx;
uint32_t old_e_tx;

/*---------------------------------------------------------------------------*/
PROCESS(sender_process, "Sender process");
AUTOSTART_PROCESSES(&sender_process);
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("SENDER: Data received on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;
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
}
/*---------------------------------------------------------------------------*/ 

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sender_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t *addr;

  PROCESS_BEGIN();

  leds_on(BLUE);

  servreg_hack_init();
  set_global_address();
  simple_udp_register(&connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  etimer_set(&periodic_timer, SEND_INTERVAL);

  old_e_cpu = energest_type_time(ENERGEST_TYPE_CPU);
  old_e_rx = energest_type_time(ENERGEST_TYPE_LISTEN);
  old_e_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    addr = servreg_hack_lookup(SERVICE_ID);
    if(addr != NULL) {
      leds_on(GREEN);
      leds_off(RED);

      SENSORS_ACTIVATE(light_sensor);

      int light_sensor1 = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
      int light_sensor2 = light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR);
      uint32_t e_cpu = energest_type_time(ENERGEST_TYPE_CPU);
      uint32_t e_rx = energest_type_time(ENERGEST_TYPE_LISTEN);
      uint32_t e_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);

      uint32_t cpu_variation = e_cpu - old_e_cpu;
      uint32_t rx_variation = e_rx - old_e_rx;
      uint32_t tx_variation = e_tx - old_e_tx;

      int32_t energy_cpu = ((double)(cpu_variation)*10*3.6)/RTIMER_ARCH_SECOND;
      int32_t energy_network = (((double)(tx_variation)*17.4*3.6)/RTIMER_ARCH_SECOND) + (((double)(rx_variation)*18.8*3.6)/RTIMER_ARCH_SECOND);

      int32_t *data = malloc(3*sizeof(int32_t));
      data[0] = light_sensor1;
      data[1] = light_sensor2;
      data[2] = energy_cpu + energy_network;

      old_e_cpu = e_cpu;
      old_e_rx = e_rx;
      old_e_tx = e_tx;  

      printf("Sending message to ");
      uip_debug_ipaddr_print(addr);
      printf("\n");

      printf("Data: [%ld, %ld, %lu]\n", data[0], data[1], data[2]);

      simple_udp_sendto(&connection, data, sizeof(int32_t)*3, addr);

      free(data);

      leds_off(GREEN);
    } else {
      leds_on(RED);
      printf("Error Sending Data: Time: %ld\n", clock_seconds());
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
