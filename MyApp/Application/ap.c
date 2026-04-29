#include "ap.h"
#include "motor.h"
#include "Scurve.h"
#include "elevator_controller.h"
#include "motion_unit_conv.h"
#include "def.h"

// inner
// static elevator_t elevator;



// task Init
void StartDefaultTask(void *argument) {
    apInit();

    while (1) {
        apMain();
    }
}

void motorTask(void *argument) {
    while (1) {
        osDelay(1);
    }
}

// function
void apInit(void) {
    bspInit();
    Elevator_Controller_Init();

    // elevatorInit(&elevator);
}

void apMain(void) {
        Elevator_Controller_Update();
        osDelay(10);
        
}
