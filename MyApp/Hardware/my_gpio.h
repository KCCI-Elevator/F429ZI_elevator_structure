#ifndef __MAP_HW__MY_GPIO_H__
#define __MAP_HW__MY_GPIO_H__

#include "def.h"
#include "hw_def.h"

// port num : 0=A, 1=B, ... , 10=K  // K[7:0]
bool gpioExtWrite(uint8_t port_idx, uint8_t pin_num, uint8_t state);
int8_t gpioExtRead(uint8_t port_idx, uint8_t pin_num);

#endif //__MAP_HW__MY_GPIO_H__