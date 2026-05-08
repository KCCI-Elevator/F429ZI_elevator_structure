#ifndef __MAP_HW__MY_GPIO_H__
#define __MAP_HW__MY_GPIO_H__

#include "def.h"
#include "hw_def.h"

/* Port index: 0=A, 1=B, ... , 10=K. */
GPIO_TypeDef *gpioGetPortPtr(uint8_t port_idx);

bool gpioExtInit(uint8_t port_idx, uint8_t pin_num, uint32_t mode);
bool gpioExtInitPull(uint8_t port_idx, uint8_t pin_num, uint32_t mode, uint32_t pull);
bool gpioExtWrite(uint8_t port_idx, uint8_t pin_num, uint8_t state);
int8_t gpioExtRead(uint8_t port_idx, uint8_t pin_num);

bool gpioPinInit(GPIO_TypeDef *port, uint16_t pin, uint32_t mode, uint32_t pull);
bool gpioPinWrite(GPIO_TypeDef *port, uint16_t pin, bool state);
int8_t gpioPinRead(GPIO_TypeDef *port, uint16_t pin);

#endif // __MAP_HW__MY_GPIO_H__
