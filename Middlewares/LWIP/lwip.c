#include "stdio.h"
#include "stm32f103xb.h"

#include "netif.h"
#include "ip_addr.h"
#include "ethernet.h"
#include "etharp.h"
#include "priv/tcp_priv.h"
#include "device.h"
#include "config.h"

extern dev_ctrl device;
static struct netif netif;
__IO uint32_t TCPTimer = 0;
__IO uint32_t ARPTimer = 0;

#define PING_ENABLE     0
#define MQTT_ENABLE     1

#if PING_ENABLE
#include "ping.h"
#endif
#if MQTT_ENABLE
#include "mqtt_client.h"
#endif


#if PING_ENABLE
ip_addr_t pingip;
__IO uint32_t PINGTimer = 0;
#endif 

#if MQTT_ENABLE
__IO uint32_t MQTTTimer = 0;
#endif

extern err_t ethernetif_init(struct netif *netif);
extern err_t ethernetif_input(struct netif *netif);
void LwIP_Init(void)
{
   ip_addr_t ipaddr;
   ip_addr_t netmask;
   ip_addr_t gw;
   
  /* Initializes the dynamic memory heap defined by MEM_SIZE.*/
  mem_init();

  /* Initializes the memory pools defined by MEMP_NUM_x.*/
  memp_init();

  IP4_ADDR(&ipaddr, LOCAL_IP_0, LOCAL_IP_1, LOCAL_IP_2, LOCAL_IP_3);
  IP4_ADDR(&netmask, NETMASK_0, NETMASK_1, NETMASK_2, NETMASK_3);
  IP4_ADDR(&gw, GATEWAY_IP_0, GATEWAY_IP_1, GATEWAY_IP_2, GATEWAY_IP_3);
  
  /* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
            struct ip_addr *netmask, struct ip_addr *gw,
            void *state, err_t (* init)(struct netif *netif),
            err_t (* input)(struct pbuf *p, struct netif *netif))
    
   Adds your network interface to the netif_list. Allocate a struct
  netif and pass a pointer to this structure as the first argument.
  Give pointers to cleared ip_addr structures when using DHCP,
  or fill them with sane numbers otherwise. The state pointer may be NULL.

  The init function pointer must point to a initialization function for
  your ethernet netif interface. The following code illustrates it's use.*/
  netif_add(&netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);

  /*  Registers the default network interface.*/
  netif_set_default(&netif);

  /*  When the netif is fully configured this function must be called.*/
  netif_set_up(&netif);
#if PING_ENABLE
  IP4_ADDR(&pingip, 192,168,1,100);
  ping_init(&pingip);
#endif
  
#if MQTT_ENABLE
    mqtt_init();
#endif
}

void LwIP_Periodic_Handle(__IO uint32_t localtime)
{
  ethernetif_input(&netif);
  /* TCP periodic process every 250 ms */
  if (localtime - TCPTimer >= TCP_TMR_INTERVAL)
  {
    TCPTimer =  localtime;
    tcp_tmr();
  }
  /* ARP periodic process every 5s */
  if (localtime - ARPTimer >= ARP_TMR_INTERVAL)
  {
    ARPTimer =  localtime;
    etharp_tmr();
  }
#if PING_ENABLE
  if (localtime - PINGTimer >= 10000) {
      PINGTimer = localtime;
      ping_send_now();
  }
#endif
  
#if MQTT_ENABLE
  switch(device.mqtt_cmd) {
  case MQTT_CMD_TYPE_LED:
    if (device.device_ctrl) {
      do_mqtt_publish("resp:led:on");
    } else {
      do_mqtt_publish("resp:led:off");
    } 
    break;
  case MQTT_CMD_TYPE_TEMP:
    if (device.device_ctrl) {
      do_mqtt_publish("resp:temp:on");
    } else {
      do_mqtt_publish("resp:temp:off");
    }
    break;
  case MQTT_CMD_TYPE_WINDOW:
    if (device.device_ctrl) {
      do_mqtt_publish("resp:window:on");
    } else {
      do_mqtt_publish("resp:window:off");
    }
    break;
  default:
    break;
  }
  device.mqtt_cmd = 0;
#endif
  
}