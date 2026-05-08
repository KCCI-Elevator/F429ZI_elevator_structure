#include "my_gpio.h"

GPIO_TypeDef *gpioGetPortPtr(uint8_t port_idx)
{
    switch (port_idx) {
    case 0:
        __HAL_RCC_GPIOA_CLK_ENABLE();
        return GPIOA;
    case 1:
        __HAL_RCC_GPIOB_CLK_ENABLE();
        return GPIOB;
    case 2:
        __HAL_RCC_GPIOC_CLK_ENABLE();
        return GPIOC;
    case 3:
        __HAL_RCC_GPIOD_CLK_ENABLE();
        return GPIOD;
    case 4:
        __HAL_RCC_GPIOE_CLK_ENABLE();
        return GPIOE;
    case 5:
        __HAL_RCC_GPIOF_CLK_ENABLE();
        return GPIOF;
    case 6:
        __HAL_RCC_GPIOG_CLK_ENABLE();
        return GPIOG;
    case 7:
        __HAL_RCC_GPIOH_CLK_ENABLE();
        return GPIOH;
    case 8:
        __HAL_RCC_GPIOI_CLK_ENABLE();
        return GPIOI;
    case 9:
        __HAL_RCC_GPIOJ_CLK_ENABLE();
        return GPIOJ;
    case 10:
        __HAL_RCC_GPIOK_CLK_ENABLE();
        return GPIOK;
    default:
        return NULL;
    }
}

static bool gpioIsValidExtPin(uint8_t port_idx, uint8_t pin_num)
{
    if (pin_num > 15U) {
        return false;
    }

    if ((port_idx == 10U) && (pin_num > 7U)) {
        return false;
    }

    return gpioGetPortPtr(port_idx) != NULL;
}

bool gpioPinInit(GPIO_TypeDef *port, uint16_t pin, uint32_t mode, uint32_t pull)
{
    GPIO_InitTypeDef gpio_init = {0};

    if (port == NULL || pin == 0U) {
        return false;
    }

    gpio_init.Pin = pin;
    gpio_init.Mode = mode;
    gpio_init.Pull = pull;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(port, &gpio_init);

    return true;
}

bool gpioExtInit(uint8_t port_idx, uint8_t pin_num, uint32_t mode)
{
    return gpioExtInitPull(port_idx, pin_num, mode, GPIO_NOPULL);
}

bool gpioExtInitPull(uint8_t port_idx, uint8_t pin_num, uint32_t mode, uint32_t pull)
{
    GPIO_TypeDef *port;

    if (gpioIsValidExtPin(port_idx, pin_num) == false) {
        return false;
    }

    port = gpioGetPortPtr(port_idx);

    return gpioPinInit(port, (uint16_t)(1UL << pin_num), mode, pull);
}

bool gpioPinWrite(GPIO_TypeDef *port, uint16_t pin, bool state)
{
    if (port == NULL || pin == 0U) {
        return false;
    }

    HAL_GPIO_WritePin(port, pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    return true;
}

bool gpioExtWrite(uint8_t port_idx, uint8_t pin_num, uint8_t state)
{
    GPIO_TypeDef *port;

    if (gpioIsValidExtPin(port_idx, pin_num) == false) {
        return false;
    }

    port = gpioGetPortPtr(port_idx);

    return gpioPinWrite(port, (uint16_t)(1UL << pin_num), state > 0U);
}

int8_t gpioPinRead(GPIO_TypeDef *port, uint16_t pin)
{
    if (port == NULL || pin == 0U) {
        return -1;
    }

    return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET ? 1 : 0;
}

int8_t gpioExtRead(uint8_t port_idx, uint8_t pin_num)
{
    GPIO_TypeDef *port;

    if (gpioIsValidExtPin(port_idx, pin_num) == false) {
        return -1;
    }

    port = gpioGetPortPtr(port_idx);

    return gpioPinRead(port, (uint16_t)(1UL << pin_num));
}
