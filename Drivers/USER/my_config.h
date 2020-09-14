#ifndef __CONFIG_H__
#define __CONFIG_H_


#define PING_ENABLE     0
#define MQTT_ENABLE     1


/*
*
*/
#define LOCAL_IP_0              192
#define LOCAL_IP_1              168
#define LOCAL_IP_2              199
#define LOCAL_IP_3              219

#define NETMASK_0               255
#define NETMASK_1               255
#define NETMASK_2               255
#define NETMASK_3               0

#define GATEWAY_IP_0            192
#define GATEWAY_IP_1            168
#define GATEWAY_IP_2            199
#define GATEWAY_IP_3            1

/*
*       当前并没有使用该定义的宏，而是从flash中读取8位唯一id，使用后6位作为mac地址
*/
#define MACADDR_0               0x40
#define MACADDR_1               0xef
#define MACADDR_2               0xab
#define MACADDR_3               0x7c
#define MACADDR_4               0x8f
#define MACADDR_5               0x9d


#endif