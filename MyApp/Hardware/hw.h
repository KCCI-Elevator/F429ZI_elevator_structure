#ifndef __MAP_HW__HW_H__
#define __MAP_HW__HW_H__

#include <stdint.h>
#include <stm32f4xx_hal.h>

void hwInit(void);

uint32_t hwMillis(void);
void hwDelay(uint32_t delay_ms);

#endif //__MAP_HW__HW_H__