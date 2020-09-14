#include "stdio.h"
#include "stm32f103xb.h"

#include "netif.h"
#include "ip_addr.h"
#include "ethernet.h"
#include "etharp.h"
#include "priv/tcp_priv.h"
#include "timeouts.h"

#include "device.h"
#include "mqtt_client.h"
#include "my_config.h"

extern dev_ctrl device;
static struct netif netif;
uint32_t TCPTimer = 0;
uint32_t ARPTimer = 0;

#if PING_ENABLE
#include "ping.h"
#endif

#if MQTT_ENABLE
#include "mqtt_client.h"
#endif

#if LWIP_DNS
#include "dns.h"

ip_addr_t dns0server;
ip_addr_t mqtt_server_ip;
uint32_t DNSTimer = 0;

static void dns_found(const char *name, const ip_addr_t *addr, void *arg)
{
  LWIP_UNUSED_ARG(arg);
  printf("%s: %s\r\n", name, addr ? ipaddr_ntoa(addr) : "<not found>");
  if (addr->addr) {
    mqtt_server_ip.addr = addr->addr;
  }
}

uint32_t lwip_rand(void)
{
  /* this is what glibc rand() returns (first 20 numbers) */
  static uint32_t rand_nrs[] = {0x6b8b4567, 0x327b23c6, 0x643c9869, 0x66334873, 0x74b0dc51,
  0x19495cff, 0x2ae8944a, 0x625558ec, 0x238e1f29, 0x46e87ccd};
//    0x3d1b58ba, 0x507ed7ab, 0x2eb141f2, 0x41b71efb, 0x79e2a9e3,
//    0x7545e146, 0x515f007c, 0x5bd062c2, 0x12200854, 0x4db127f8};
  static unsigned idx = 0;
  uint32_t ret = rand_nrs[idx];
  idx++;
  if (idx >= sizeof(rand_nrs)/sizeof((rand_nrs)[0])) {
    idx = 0;
  }
  return ret;
}
#endif

#if PING_ENABLE
ip_addr_t pingip;
uint32_t PINGTimer = 0;
#endif 

#if MQTT_ENABLE
__IO uint32_t MQTTTimer = 0;
static uint8_t mqtt_start = 0;
#endif

#if LWIP_DHCP
#include "dhcp.h"
uint32_t DHCP_500MS_Timer = 0;
uint32_t DHCP_60S_Timer = 0;
uint32_t ACDTimer = 0;
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

#if LWIP_DHCP
  IP4_ADDR(&ipaddr, 0, 0, 0, 0);
  IP4_ADDR(&netmask, 0, 0, 0, 0);
  IP4_ADDR(&gw, 0, 0, 0, 0);  
#else
  IP4_ADDR(&ipaddr, LOCAL_IP_0, LOCAL_IP_1, LOCAL_IP_2, LOCAL_IP_3);
  IP4_ADDR(&netmask, NETMASK_0, NETMASK_1, NETMASK_2, NETMASK_3);
  IP4_ADDR(&gw, GATEWAY_IP_0, GATEWAY_IP_1, GATEWAY_IP_2, GATEWAY_IP_3);
#endif
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
  
#if LWIP_DNS
  dns_init();
  IP4_ADDR(&dns0server, 114,114,114,114);
  dns_setserver(0, &dns0server);
#endif
  
#if LWIP_DHCP
   dhcp_start(&netif);
#endif
}

uint8_t is_get_mqttserver_ip_start = 0;
uint32_t getDNSTimeoutTimer = 0;

void LwIP_Periodic_Handle(__IO uint32_t localtime)
{
  ethernetif_input(&netif);
  //sys_check_timeouts();
  
  if (netif.ip_addr.addr) {
    if (!mqtt_server_ip.addr) {
      if (!is_get_mqttserver_ip_start) {
        printf("got ip addr:%s\r\n",  ipaddr_ntoa(&netif.ip_addr) );
#if LWIP_DNS
        dns_gethostbyname(MQTT_SERVER, &mqtt_server_ip, dns_found, 0);
#endif
        is_get_mqttserver_ip_start = 1;
      }
    }
  }
  
//  /* TCP periodic process every 250 ms */
  if (localtime - TCPTimer >= TCP_TMR_INTERVAL)
  {
    TCPTimer = localtime;
    tcp_tmr();
  }
  /* ARP periodic process every 5s */
  if (localtime - ARPTimer >= ARP_TMR_INTERVAL)
  {
    ARPTimer =  localtime;
    etharp_tmr();
  }
//#if PING_ENABLE
//  if (localtime - PINGTimer >= 10000) {
//      PINGTimer = localtime;
//      ping_send_now();
//  }
//#endif`

#if LWIP_DNS
  if (localtime - DNSTimer >= DNS_TMR_INTERVAL) {
    DNSTimer = localtime;
    dns_tmr();
  }
#endif
  
#if LWIP_DHCP
  if (localtime - DHCP_60S_Timer >= DHCP_COARSE_TIMER_MSECS) {
    DHCP_60S_Timer = localtime;
    dhcp_coarse_tmr();
  }
  if (localtime - DHCP_500MS_Timer >= DHCP_FINE_TIMER_MSECS) {
    DHCP_500MS_Timer = localtime;
    dhcp_fine_tmr();
  }
  if (localtime - ACDTimer >= ACD_TMR_INTERVAL) {
    ACDTimer = localtime;
    acd_tmr();
  }
#endif

#if MQTT_ENABLE
    if (mqtt_server_ip.addr) {
      if (!mqtt_start) {
#if LWIP_DNS
        IP4_ADDR(&mqtt_server_ip, 192, 168, 199, 211);
        mqtt_init(&mqtt_server_ip);
#else
        mqtt_init();
#endif //DNS_ENABLE
        mqtt_start = 1;
      } else {
        sys_check_timeouts();
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
      }
    }
#endif //MQTT_ENABLE
}
