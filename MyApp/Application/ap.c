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
    // 1. 태스크가 시작될 때의 현재 OS 틱 시간을 기록
    uint32_t tick_count = osKernelGetTickCount(); 

    while (1) {
        // 모터 제어 알고리즘 실행
        Elevator_Controller_Update();
        
        // 2. 연산 시간을 보상
        tick_count += 10U; 
        osDelayUntil(tick_count); 
    }
}

// function
void apInit(void) {
    bspInit();
    Elevator_Controller_Init();

}

void apMain(void) {
    if (!Elevator_IsBusy()) { 
        
        static int sequence = 0; 
        
        osDelay(2000); 

        if (sequence == 0) {
            Elevator_GoToFloor(3);
            sequence++;
        } 
        else if (sequence == 1) {
            Elevator_GoToFloor(1);
            sequence++;
        }
        else if (sequence == 2) {
            Elevator_GoToFloor(2); 
            sequence++;            // 🎯 다음 시퀀스로 넘김
        }
        else if (sequence == 3) {
            Elevator_GoToFloor(1);
            sequence = 0;          // 🎯 1층 복귀 후 처음(0)으로 초기화
        }
    }
    osDelay(10);
}
