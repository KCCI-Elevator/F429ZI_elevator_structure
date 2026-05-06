#include "fan_control.h"
#include "my_gpio.h"

// CubeMX currently configures PB8/PB9 as outputs.
#define FAN_PORT 1 // PORT B
#define IN1_PIN  8
#define IN2_PIN  9

void Fan_Init(void) {
    gpioExtInit(FAN_PORT, IN1_PIN, GPIO_MODE_OUTPUT_PP);
    gpioExtInit(FAN_PORT, IN2_PIN, GPIO_MODE_OUTPUT_PP);
    Fan_Stop(); // 초기에는 정지 상태
}

void Fan_Start(void) {
    gpioExtWrite(FAN_PORT, IN1_PIN, 1);
    gpioExtWrite(FAN_PORT, IN2_PIN, 0);
}

void Fan_Stop(void) {
    gpioExtWrite(FAN_PORT, IN1_PIN, 0);
    gpioExtWrite(FAN_PORT, IN2_PIN, 0);
}