#include "stdio.h"
#include "stm32f103xb.h"

#include "netif.h"
#include "ip_addr.h"
#include "ethernet.h"
#include "etharp.h"
#include "priv/tcp_priv.h"
#include "timeouts.h"

#include "device.h"
#include "my_config.h"

extern dev_ctrl device;
static struct netif netif;
__IO uint32_t TCPTimer = 0;
__IO uint32_t ARPTimer = 0;

#if PING_ENABLE
#include "ping.h"
#endif

#if MQTT_ENABLE
#include "mqtt_client.h"
#endif

#if DNS_ENABLE
#include "dns.h"
#define DNS_NAME      "mqtt.iotwonderful.cn"

ip_addr_t mqtt_server_ip;

static uint8_t is_get_ipaddr_is_ok = 0;
__IO uint32_t DNSTimer = 0;

static void dns_found(const char *name, const ip_addr_t *addr, void *arg)
{
  LWIP_UNUSED_ARG(arg);
  printf("%s: %s\r\n", name, addr ? ipaddr_ntoa(addr) : "<not found>");
  is_get_ipaddr_is_ok = 1;
  mqtt_server_ip.addr = addr->addr;
}

static void dns_dorequest(void)
{
  ip_addr_t dnsresp;
  if (dns_gethostbyname(DNS_NAME, &dnsresp, dns_found, 0) == ERR_OK) {
    dns_found(DNS_NAME, &dnsresp, 0); 
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
__IO uint32_t PINGTimer = 0;
#endif 

#if MQTT_ENABLE
__IO uint32_t MQTTTimer = 0;
static uint8_t mqtt_start = 0;
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
  
#if DNS_ENABLE
  dns_init();
  dns_dorequest();
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
  
#if DNS_ENABLE
  if (localtime - DNSTimer >= 5000) {
    DNSTimer = localtime;
    if (!is_get_ipaddr_is_ok) {
      dns_tmr();
    }
  }
#endif
  
#if MQTT_ENABLE
#if DNS_ENABLE
  if (is_get_ipaddr_is_ok) {
#endif //DNS_ENABLE
    if (!mqtt_start) {
#if DNS_ENABLE
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
#if DNS_ENABLE
  }
#endif//DNS_ENABLE
#endif//MQTT_ENABLE
  
}