#ifndef __MQTT_CLIENT_MY_H__
#define __MQTT_CLIENT_MY_H__

#include "ip_addr.h"
#include "config.h"

#if DNS_ENABLE
void mqtt_init(ip_addr_t *serverip);
#else
void mqtt_init(void);
#endif

void do_mqtt_publish(char *pub_payload);

#endif