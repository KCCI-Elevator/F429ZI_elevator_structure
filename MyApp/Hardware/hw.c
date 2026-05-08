#include "hw.h"

#include "my_pwm.h"
#include "my_uart.h"
#include "my_encoder.h"

void hwInit(void){
    // User Hardware Init
    myPwmInit();
    myEncoderInit();
    uartInit();
}

uint32_t hwMillis(void){
    return HAL_GetTick();
}

void hwDelay(uint32_t delay_ms){
    osDelay(delay_ms);
}