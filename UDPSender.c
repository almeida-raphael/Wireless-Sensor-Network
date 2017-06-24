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
#include "dev/sht11/sht11-sensor.h"
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
#define UDP_PORT 1234
#define SERVICE_ID 190

#define SEND_INTERVAL   (60 * CLOCK_SECOND)
#define SEND_TIME   (random_rand() % (SEND_INTERVAL))
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static struct simple_udp_connection connection;
/*---------------------------------------------------------------------------*/

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

  leds_on(LEDS_BLUE);

  servreg_hack_init();
  set_global_address();
  simple_udp_register(&connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  etimer_set(&periodic_timer, SEND_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    addr = servreg_hack_lookup(SERVICE_ID);
    if(addr != NULL) {
      leds_on(LEDS_GREEN);
      leds_off(LEDS_RED);

      SENSORS_ACTIVATE(light_sensor);
      SENSORS_ACTIVATE(sht11_sensor);

      uint32_t data[] = {
        light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC),
        light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR),
        sht11_sensor.value(SHT11_SENSOR_TEMP),
        sht11_sensor.value(SHT11_SENSOR_HUMIDITY),
        energest_type_time(ENERGEST_TYPE_LPM),
        energest_type_time(ENERGEST_TYPE_CPU),
        energest_type_time(ENERGEST_TYPE_LISTEN),
        energest_type_time(ENERGEST_TYPE_TRANSMIT),
        energest_type_time(ENERGEST_TYPE_LED_RED)
      };

      printf("Sending message to ");
      uip_debug_ipaddr_print(addr);
      printf("\n");

      printf("Data: [%u, %u, %u, %u, %lu, %lu, %lu, %lu, %lu]\n",
        data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]
      );

      simple_udp_sendto(&connection, data, sizeof(data), addr);

      leds_off(LEDS_GREEN);
    } else {
      leds_on(LEDS_RED);
      printf("Error Sending Data");
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
