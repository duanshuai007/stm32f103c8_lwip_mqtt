#ifndef LWIP_HDR_LWIPOPTS_H
#define LWIP_HDR_LWIPOPTS_H

#define NO_SYS                        1
#define LWIP_IPV4                       1
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0
//
#define NO_SYS_NO_TIMERS                1
//
#define SYS_LIGHTWEIGHT_PROT            0
//#define LWIP_DHCP     0
//
//#define MEM_LIBC_MALLOC                 0
//#define MEM_OVERFLOW_CHECK              0
//#define MEMP_OVERFLOW_CHECK             0
//
#define LWIP_RAW        1
//#define PPP_SUPPORT     0
//
#define LWIP_TIMERS     1
#define LWIP_DNS        1
#define DNS_TABLE_SIZE  2
#define DNS_MAX_NAME_LENGTH     32
#define DNS_MAX_SERVERS 1
#define LWIP_RAND       lwip_rand
#define DNS_SERVER_ADDRESS(ipaddr) (ip4_addr_set_u32(ipaddr, ipaddr_addr("114.114.114.114")))

#define LWIP_TCP        1
#define LWIP_UDP        1
//for mdns
//#define MEMP_NUM_UDP_PCB                2
//#define LWIP_NUM_NETIF_CLIENT_DATA      1

#define LWIP_ARP                        1
#define ARP_TABLE_SIZE                  4
#define ARP_MAXAGE                      300
#define LWIP_ETHERNET                   1

//for mqtt
#define LWIP_CALLBACK_API               1
#define MEMP_NUM_SYS_TIMEOUT            10

//#define ETHARP_SUPPORT_VLAN             1
#define MEM_SIZE        (512)
#define PBUF_POOL_SIZE  5
#define PBUF_POOL_BUFSIZE 300
#define TCP_MSS         (300 - 40)
#define LWIP_ICMP               0
#define LWIP_IGMP               0

#define MEMP_NUM_TCP_PCB_LISTEN         4
//#define MEMP_NUM_TCP_SEG                TCP_SBD_QUEUELEN

//for mqtt ssl
#define LWIP_ALTCP              1
#define LWIP_ALTCP_TLS          1
#define LWIP_ALTCP_TLS_MBEDTLS  1
#define ALTCP_MBEDTLS_RNG_FN    mbedtls_entropy_func

#define LWIP_DEBUG      1
#define ETHARP_DEBUG    LWIP_DBG_OFF
#define ICMP_DEBUG      LWIP_DBG_OFF
#define IP_DEBUG        LWIP_DBG_OFF
#define DNS_DEBUG       LWIP_DBG_OFF
#define MQTT_DEBUG      LWIP_DBG_OFF
#define TCP_DEBUG       LWIP_DBG_OFF
#define TCP_INPUT_DEBUG LWIP_DBG_OFF

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

#endif /* LWIP_HDR_LWIPOPTS_H */