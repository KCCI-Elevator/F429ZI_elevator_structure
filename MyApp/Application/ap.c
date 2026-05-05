#include "ap.h"
#include "motor.h"
#include "Scurve.h"
#include "elevator_controller.h"
#include "motion_unit_conv.h"
#include "def.h"

// inner
// static elevator_t elevator;

extern TIM_HandleTypeDef htim6;

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

    HAL_TIM_Base_Start_IT(&htim6);
}

void apMain(void) {
        // Elevator_Controller_Update();
        osDelay(10);
        
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // 우리가 설정한 제어 루프용 TIM6인지 확인
    if (htim->Instance == TIM6) {
        
        // 10ms 마다 모터 제어 로직 실행
        Elevator_Controller_Update(); 
        
    }
}