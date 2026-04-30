#include "my_gpio.h"

// inner
static GPIO_TypeDef *getPortPtr(uint8_t port_idx) {
    // nucleo-F429ZI 보드에 맞는 GPIO핀 설정: A-K // A-J[15:0], K[7:0]
    switch (port_idx) {
        case 0:
            __HAL_RCC_GPIOA_CLK_ENABLE();
            return GPIOA; // 15:0
        case 1:
            __HAL_RCC_GPIOB_CLK_ENABLE();
            return GPIOB; // 15:0
        case 2:
            __HAL_RCC_GPIOC_CLK_ENABLE();
            return GPIOC; // 15:0
        case 3:
            __HAL_RCC_GPIOD_CLK_ENABLE();
            return GPIOD; // 15:0
        case 4:
            __HAL_RCC_GPIOE_CLK_ENABLE();
            return GPIOE; // 15:0
        case 5:
            __HAL_RCC_GPIOF_CLK_ENABLE();
            return GPIOF; // 15:0
        case 6:
            __HAL_RCC_GPIOG_CLK_ENABLE();
            return GPIOG; // 15:0
        case 7:
            __HAL_RCC_GPIOH_CLK_ENABLE();
            return GPIOH; // 15:0
        case 8:
            __HAL_RCC_GPIOI_CLK_ENABLE();
            return GPIOI; // 15:0
        case 9:
            __HAL_RCC_GPIOJ_CLK_ENABLE();
            return GPIOJ; // 15:0
        case 10:
            __HAL_RCC_GPIOK_CLK_ENABLE();
            return GPIOK; // 7:0
        default:
            return NULL;
    }
}

// function
// port num : 0=A, 1=B, ... , 10=K  // K[7:0]

void gpioExtInit(uint8_t port_idx, uint8_t pin_num, uint32_t mode) {
    if (pin_num > 15) return;
    if ((port_idx == 10) && (pin_num > 7)) return;

    GPIO_TypeDef *pPort = getPortPtr(port_idx);
    if (pPort == NULL) return;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = (1 << pin_num);
    GPIO_InitStruct.Mode = mode; // 예: GPIO_MODE_OUTPUT_PP
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(pPort, &GPIO_InitStruct);
}


bool gpioExtWrite(uint8_t port_idx, uint8_t pin_num, uint8_t state) {
    GPIO_TypeDef *pPort = getPortPtr(port_idx);
    if (pPort == NULL) return false;

    uint16_t pin_mask = (1 << pin_num);
    HAL_GPIO_WritePin(pPort, pin_mask, (state > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return true;
}

int8_t gpioExtRead(uint8_t port_idx, uint8_t pin_num) {
    // 1: high, 0: low, -1: error
    if (pin_num > 15) return -1; // error
    if ((port_idx == 10) && (pin_num > 7)) return -1;

    // 입력, 출력 두 가지 모드를 구분해야함
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = (1 << pin_num);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(getPortPtr(port_idx), &GPIO_InitStruct);

    GPIO_TypeDef *pPort = getPortPtr(port_idx);

    if (pPort == NULL) return -1; // error

    uint16_t pin_mask = (1 << pin_num);

    return HAL_GPIO_ReadPin(pPort, pin_mask) == GPIO_PIN_SET ? 1 : 0;
}