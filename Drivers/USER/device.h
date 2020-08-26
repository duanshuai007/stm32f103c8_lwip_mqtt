#ifndef __DEVICE_STRUCT_H__
#define __DEVICE_STRUCT_H__

#include "stm32f103xb.h"

#define MQTT_CMD_TYPE_LED       1
#define MQTT_CMD_TYPE_TEMP      2
#define MQTT_CMD_TYPE_WINDOW    3
#define DEVICE_OPEN             1
#define DEVICE_CLOSE            0

typedef struct dev_ctrl {
    __IO uint8_t mqtt_cmd;
    __IO uint8_t device_ctrl;
//    uint8_t mqtt_resp;
} dev_ctrl;

#endif