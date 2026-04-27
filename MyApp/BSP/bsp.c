#include "bsp.h"

// inner
// static bsp_elevator_input_t elevator_input;

// function
void bspInit(void){
    // memset(&elevator_input, 0, sizeof(elevator_input));

    hwInit();
    motorInit();    // from motor.c ???

}

void bspUpdate(void) {}

uint32_t bspMillis(void){
    return hwMillis();
}
void bspDelay(uint32_t delay_ms){
    hwDelay(delay_ms);
}
