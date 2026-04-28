#include "hw.h"

void hwInit(void){
    // User Driver Init
}

uint32_t hwMillis(void){
    return HAL_GetTick();
}

void hwDelay(uint32_t delay_ms){
    osDelay(delay_ms);
}