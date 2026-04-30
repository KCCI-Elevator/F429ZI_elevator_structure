#include "ap.h"
#include "motor.h"
#include "Scurve.h"
#include "elevator_controller.h"
#include "motion_unit_conv.h"
#include "def.h"
#include "elevator.h"
#include "elevator_io.h"
#include "bsp.h"

// inner
static elevator_t elevator;



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
    elevatorInit(&elevator);

    // elevatorInit(&elevator);
}

void apMain(void) {
    uint32_t prev_time = bspMillis();
    while(1) {
        //UART 입력 감시 (최대한 자주 확인)
        Elevator_IO_Update(&elevator);

        //10ms 주기 제어 루프
        uint32_t now = bspMillis();
        if(now - prev_time >= 10) {
            prev_time = now;
            bspUpdate();
            elevatorUpdate(&elevator,now);
        }
        
    }
        
        
}
