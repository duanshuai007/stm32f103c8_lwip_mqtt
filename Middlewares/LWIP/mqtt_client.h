#ifndef __MQTT_CLIENT_MY_H__
#define __MQTT_CLIENT_MY_H__

#include "ip_addr.h"
#include "my_config.h"
#include "lwipopts.h"

#define MQTT_MSG_ONLINE         "online"
#define MQTT_MSG_OFFLINE        "offline"
#define MQTT_SERVER             "mqtt.iotwonderful.cn"
#define MQTT_SERVER_PORT        1883
#define MQTT_KEEPALIVE          60
#define MQTT_USER               "test_001"
#define MQTT_PASSWORD           "NjBlNjY3ZWRlZ"


#if LWIP_DNS
void mqtt_init(ip_addr_t *serverip);
#else
void mqtt_init(void);
#endif

void do_mqtt_publish(char *pub_payload);

#endif