#include "lwip/sys.h"
#include "sys_arch.h"
#include "stm32f103xb.h"

extern __IO u32_t LocalTime;

u32_t sys_now(void)
{
    return LocalTime;
}