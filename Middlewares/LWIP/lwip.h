#ifndef __MY_LWIP_H__
#define __MY_LWIP_H__

#include "stdio.h"
#include "stm32f103xb.h"

void LwIP_Init(void);
void LwIP_Periodic_Handle(__IO uint32_t localtime);


#endif