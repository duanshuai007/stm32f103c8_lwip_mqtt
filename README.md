#stm32f103c8+enc28j60 lwip+mqtt

##enc28j60 踩坑

enc28j60的驱动是从网上找到的别人已经写好的驱动直接拿来用的，但是在用过的过程中自己自作聪明修改了一些地方，结果导致网卡只能发送数据，无法接受数据。

```
void enc28j60_init(uint8_t *macaddr) {
		
	...
		
	enc28j60Write(MAADR5, macaddr[0]);	
	enc28j60Write(MAADR4, macaddr[1]);
	enc28j60Write(MAADR3, macaddr[2]);
	enc28j60Write(MAADR2, macaddr[3]);
	enc28j60Write(MAADR1, macaddr[4]);
	enc28j60Write(MAADR0, macaddr[5]);
  
	...
  
} 

static void
low_level_init(struct netif *netif)
{
  //struct ethernetif *ethernetif = netif->state;

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = macaddr[0];
  netif->hwaddr[1] = macaddr[1];
  netif->hwaddr[2] = macaddr[2];
  netif->hwaddr[3] = macaddr[3];
  netif->hwaddr[4] = macaddr[4];
  netif->hwaddr[5] = macaddr[5];
  ...
}
```

在这两处有对网卡的mac地址硬件设置以及设计协议栈中mac地址的地方，此两处必须保持这种对应的关系，否则网卡就会接收不到数据。

## lwip移植

lwip的移植工作很简单，首先下载git代码.

```
git clone git://git.savannah.gnu.org/lwip.git 
```

将`src`目录中的`core`,`netif`,`include`分别复制到`stm32`工程的目录下面，具体放在哪个目录下面都可以。然后设置包含头文件路径。这里可以注意一下，尽量不要设置为绝对路径，绝对路径在你复制了工程拿到别的电脑上时就没办法运行了，所以尽量设置为相对路径。

```
比如：
$PROJ_DIR$/../Drivers/USER
在这里$PROJ_DIR$的内容其实就是你的stm32工程文件所在的目录，在使用iar的工程中就是Project.eww所在的目录,以我的工程来说是C:\stm32\stm32project\stm32f103c8_lwip\EWARM
```
  
因为我将以上三个文件见放在了`C:\stm32\stm32project\stm32f103c8_lwip\Middlewares\LWIP`目录下

所以我包含的头文件路径如下
```
$PROJ_DIR$/../Middlewares/LWIP/include
$PROJ_DIR$/../Middlewares/LWIP/include/lwip
$PROJ_DIR$/../Middlewares/LWIP/include/netif
$PROJ_DIR$/../Middlewares/LWIP
```

将`lwip/contrib/ports/win32/include/arch`目录复制到我们项目的`LWIP/include/`目录下。
因为我们是无rtos的移植，所以将sys_arch.h内容清空，只保留一个空文件。
然后在`LWIP/include`目录下创建sys_arch.c文件，并在其中写入以下内容.

```
#include "lwip/sys.h"
#include "sys_arch.h"
 
u32_t sys_now(void)
{
    return 0;
}
```

打开`LWIP/include/cc.h`文件，将其中的内容做如下的修改。

```
/* Define an example for LWIP_PLATFORM_DIAG: since this uses varargs and the old
* C standard lwIP targets does not support this in macros, we have extra brackets
* around the arguments, which are left out in the following macro definition:
*/
#if !defined(LWIP_TESTMODE) || !LWIP_TESTMODE
//void lwip_win32_platform_diag(const char *format, ...);
//#define LWIP_PLATFORM_DIAG(x) lwip_win32_platform_diag x

#endif

//extern unsigned int lwip_port_rand(void);
//#define LWIP_RAND() ((uint32_t)lwip_port_rand())

//#define PPP_INCLUDE_SETTINGS_HEADER
```	

在`LWIP/include`目录下新建文件`lwipopts.h`，从名字可以看出这是配置lwip配置选项的文件。
我的配置文件是参考了其他别人的移植成果，并经过自己的测试。内容如下。

```
#ifndef LWIP_HDR_LWIPOPTS_H
#define LWIP_HDR_LWIPOPTS_H


/*----- Value in opt.h for MEM_ALIGNMENT: 1 -----*/
#define MEM_ALIGNMENT 4
/*----- Default Value for MEMP_NUM_PBUF: 16 ---*/
#define MEMP_NUM_PBUF 4
/*----- Value in opt.h for LWIP_DNS_SECURE: (LWIP_DNS_SECURE_RAND_XID | LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING | LWIP_DNS_SECURE_RAND_SRC_PORT) -*/
#define LWIP_DNS_SECURE 1
/*----- Value in opt.h for TCP_SND_QUEUELEN: (4*TCP_SND_BUF + (TCP_MSS - 1))/TCP_MSS -----*/
//#define TCP_SND_QUEUELEN 9
/*----- Value in opt.h for TCP_SNDLOWAT: LWIP_MIN(LWIP_MAX(((TCP_SND_BUF)/2), (2 * TCP_MSS) + 1), (TCP_SND_BUF) - 1) -*/
//#define TCP_SNDLOWAT 1071
/*----- Value in opt.h for TCP_SNDQUEUELOWAT: LWIP_MAX(TCP_SND_QUEUELEN)/2, 5) -*/
#define TCP_SNDQUEUELOWAT 5
/*----- Value in opt.h for TCP_WND_UPDATE_THRESHOLD: LWIP_MIN(TCP_WND/4, TCP_MSS*4) -----*/
#define TCP_WND_UPDATE_THRESHOLD 536
/*----- Value in opt.h for LWIP_NETCONN: 1 -----*/
#define LWIP_NETCONN 0
/*----- Value in opt.h for LWIP_SOCKET: 1 -----*/
#define LWIP_SOCKET 0
/*----- Value in opt.h for RECV_BUFSIZE_DEFAULT: INT_MAX -----*/
//#define RECV_BUFSIZE_DEFAULT 1500
/*----- Value in opt.h for LWIP_STATS: 1 -----*/
#define LWIP_STATS 0
/*----- Value in opt.h for CHECKSUM_GEN_IP: 1 -----*/
#define CHECKSUM_GEN_IP 1
/*----- Value in opt.h for CHECKSUM_GEN_UDP: 1 -----*/
#define CHECKSUM_GEN_UDP 1
/*----- Value in opt.h for CHECKSUM_GEN_TCP: 1 -----*/
#define CHECKSUM_GEN_TCP 1
/*----- Value in opt.h for CHECKSUM_GEN_ICMP: 1 -----*/
#define CHECKSUM_GEN_ICMP 1
/*----- Value in opt.h for CHECKSUM_GEN_ICMP6: 1 -----*/
#define CHECKSUM_GEN_ICMP6 0
/*----- Value in opt.h for CHECKSUM_CHECK_IP: 1 -----*/
#define CHECKSUM_CHECK_IP 1
/*----- Value in opt.h for CHECKSUM_CHECK_UDP: 1 -----*/
#define CHECKSUM_CHECK_UDP 1
/*----- Value in opt.h for CHECKSUM_CHECK_TCP: 1 -----*/
#define CHECKSUM_CHECK_TCP 1
/*----- Value in opt.h for CHECKSUM_CHECK_ICMP: 1 -----*/
#define CHECKSUM_CHECK_ICMP 0
/*----- Value in opt.h for CHECKSUM_CHECK_ICMP6: 1 -----*/
#define CHECKSUM_CHECK_ICMP6 0

#define NO_SYS                        1
#define LWIP_IPV4                       1
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0
#define NO_SYS_NO_TIMERS                1
#define SYS_LIGHTWEIGHT_PROT            0
#define LWIP_RAW        1
//
#define LWIP_TIMERS     1
#define LWIP_DNS        1
#define LWIP_DHCP       1       
#define DNS_TABLE_SIZE  2
#define DNS_MAX_NAME_LENGTH     32
#define DNS_MAX_SERVERS 1
#define LWIP_RAND       lwip_rand

#define LWIP_TCP        1
#define LWIP_UDP        1

#define LWIP_ARP                        1
#define ARP_TABLE_SIZE                  2
#define LWIP_ETHERNET                   1

//for mqtt
#define LWIP_CALLBACK_API               1
#define MEMP_NUM_SYS_TIMEOUT            10
#define MQTT_OUTPUT_RINGBUF_SIZE        200
#define MQTT_VAR_HEADER_BUFFER_LEN      200

#define MEM_SIZE                        (2*1024)
#define MEMP_NUM_TCP_PCB                4
#define MEMP_NUM_TCP_PCB_LISTEN         2
#define MEMP_NUM_TCP_SEG                TCP_SND_QUEUELEN
#define PBUF_POOL_SIZE                  4
#define LWIP_ICMP               0
#define LWIP_IGMP               0

//for mqtt ssl
//#define LWIP_ALTCP              1
//#define LWIP_ALTCP_TLS          1
//#define LWIP_ALTCP_TLS_MBEDTLS  1
//#define ALTCP_MBEDTLS_RNG_FN    mbedtls_entropy_func

#define LWIP_DEBUG      0
#define ETHARP_DEBUG    LWIP_DBG_OFF
#define ICMP_DEBUG      LWIP_DBG_OFF
#define IP_DEBUG        LWIP_DBG_OFF
#define DNS_DEBUG       LWIP_DBG_OFF
#define MQTT_DEBUG      LWIP_DBG_OFF
#define TCP_DEBUG       LWIP_DBG_OFF
#define UDP_DEBUG       LWIP_DBG_OFF   
#define ACD_DEBUG       LWIP_DBG_OFF
#define TCP_INPUT_DEBUG LWIP_DBG_OFF
#define DHCP_DEBUG      LWIP_DBG_OFF

#define LWIP_TCPIP_CORE_LOCKING         0
#define MEMP_NUM_FRAG_PBUF              4
#define MEMP_NUM_ARP_QUEUE              4
#define MEMP_NUM_NETCONN                2
#define MEMP_NUM_SELECT_CB              2
#define MEMP_NUM_TCPIP_MSG_API          2
#define MEMP_NUM_TCPIP_MSG_INPKT        2

#endif /* LWIP_HDR_LWIPOPTS_H */
```

将lwip源码中的`contrib/examples/ethernetif/ethernetif.c`文件复制到工程目录`Middlewares/LWIP`目录下。

该文件就是enc28j60与lwip这两个硬件与软件之间的接口，我们只需要修改该文件，即可实现enc28j60的数据与lwip软件层面的互通。

需要修改的主要有以下几点:

### 1. `low_level_init`函数

文件中只展示了新增和修改的内容。

```
static void
low_level_init(struct netif *netif)
{
	...
  netif->hwaddr[0] = macaddr[0];
  netif->hwaddr[1] = macaddr[1];
  netif->hwaddr[2] = macaddr[2];
  netif->hwaddr[3] = macaddr[3];
  netif->hwaddr[4] = macaddr[4];
  netif->hwaddr[5] = macaddr[5];
	...
	
  /* Do whatever else is needed to initialize interface. */
  enc28j60Init(netif->hwaddr);
  enc28j60PhyWrite(PHLCON, 0x476);
  enc28j60clkout(1);
}
```

### 2. `low_level_output`函数

```
	
static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  //struct ethernetif *ethernetif = netif->state;
  struct pbuf *q;
  uint16_t i = 0;


#if ETH_PAD_SIZE
  pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

  for (q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */
    //send data from(q->payload, q->len);
    memcpy(&net_data_send_buffer[i], (uint8_t *)q->payload, q->len);
    i = i + q->len;
  }

  enc28j60PacketSend(i, net_data_send_buffer);
  //signal that packet should be sent();
  
  ...

}

```

### 3. `low_level_input`函数

```

static struct pbuf *
low_level_input(struct netif *netif)
{
  //struct ethernetif *ethernetif = netif->state;
  struct pbuf *p, *q;
  u16_t len;
  u16_t i = 0;

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  len = enc28j60PacketReceive(500, net_data_buffer);
  if (len == 0)
    return 0;
  
  ...
  
      /* We iterate over the pbuf chain until we have read the entire
     * packet into the pbuf. */
    for (q = p; q != NULL; q = q->next) {
      /* Read enough bytes to fill this pbuf in the chain. The
       * available data in the pbuf is given by the q->len
       * variable.
       * This does not necessarily have to be a memcpy, you can also preallocate
       * pbufs for a DMA-enabled MAC and after receiving truncate it to the
       * actually received size. In this case, ensure the tot_len member of the
       * pbuf is the sum of the chained pbuf len members.
       */
      //read data into(q->payload, q->len);
      memcpy((uint8_t *)q->payload, &net_data_buffer[i], q->len);
      i = i + q->len;
    }
    
    if (i != p->tot_len)
      return 0;
      
   ...
   
}

```

### 4. `ethernetif_input`函数

修改了该函数的定义类型，其实这里也可以不用修改，但是需要在别的文件中调用该函数，所以至少要修改该函数为非静态函数，同时还要将在文件头部声明的该函数的函数类型也对应修改，否则会报错。

```
err_t ethernetif_input(struct netif *netif) {
	  err_t err = ERR_OK;
	  
	  ...
	  
	  return err;
}

```

### 5. `ethernetif_init`函数


```
err_t
ethernetif_init(struct netif *netif)
{
	...
	
	MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 0);
	...
}

```

这样`ethernetif.c`文件就修改完成了。

移植到此，可以先烧写测试一下，`lwip.c`文件是测试用的代码，执行该函数的`LwIP_Init`来初始化`lwip`协议栈，然后定期执行`LwIP_Periodic_Handle`来处理网卡数据。
如果移植没问题的话，此时能ping通该开发板。

为了能够周期执行`LwIP_Periodic_Handle `,修改`stm32f1xx_it.c`

```
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  /* USER CODE BEGIN SysTick_IRQn 1 */
  LocalTime += 1;
  /* USER CODE END SysTick_IRQn 1 */
}
```

* 这里有一点注意的，因为我用的是`stm32 ll`库函数，该库并没有默认是能`systick`中断功能，所以需要在初始化时调用`LL_SYSTICK_EnableIT()`来是能`systick`中断。


# MQTT功能

lwip的源码中`src/apps/mqtt/mqtt.c`文件提供了对mqtt功能的实现，我们将该文件放到我们的工程目录下，然后将`src/include/lwip/apps/mqtt.h mqtt_priv.h mqtt_opts.h`三个文件也放到工程目录下，然后修改`mqtt_opts.h`文件，因为stm32f103c8t6的内存真是太有限了，所以将里面的各种buffer，len什么的都改小一点，这就要求我们发送的mqtt命令一定不能超出这个界限，否则就处理不了了。

然后实现mqtt客户端的代码`mqtt_client.c`文件，就是一个简单的实现.

* 需要注意，开发板publish的消息一定不能是qos=2的，这会导致内存不够用。所以无论什么消息，都用qos=0来进行发送。


## DHCP:

使能`#define LWIP_DHCP 1`

因为我的lwip版本是1.4，所以需要自己把定时任务加入到住循环中定期执行。
首先加入
在`Lwip_Init()`中加入`dhcp_start(&netif)`

然后在主循环中加入:

```

  if (localtime - DHCP_60S_Timer >= 60000) {
    DHCP_60S_Timer = localtime;
    dhcp_coarse_tmr();
  }
  if (localtime - DHCP_500MS_Timer >= 500) {
    DHCP_500MS_Timer = localtime;
    dhcp_fine_tmr();
  }
```

  因为opt.h文件中默认使能了LWIP_DHCP_DOES_ACD_CHECK,查看源码发现在acd.c中也有一个定时任务acd_tmr(),我们把该任务也加入到主循环中。
  这样就能够获取到动态IP了。


## DNS:

使能`#define LWIP_DNS 	1`

在`Lwip_Init()`中加入

```
dns_init();
IP4_ADDR(&dns0server, 114,114,114,114);
dns_setserver(0, &dns0server);
```

然后在主循环中加入

```


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
  if (dns_gethostbyname(MQTT_SERVER_NAME, &dnsresp, dns_found, 0) == ERR_OK) {
    dns_found(MQTT_SERVER_NAME, &dnsresp, 0); 
  }
}
```

在主循环中加入dns_tmr()

```
if (localtime - DNSTimer >= DNS_TMR_INTERVAL) {
	DNSTimer = localtime;
	dns_tmr();
}
```









