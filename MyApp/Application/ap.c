#include "ap.h"
#include "motor.h"
#include "Scurve.h"
#include "elevator_controller.h"
#include "motion_unit_conv.h"
#include "def.h"
#include "ina219_test.h"
#include "bsp.h"
#include "elevator_monitor.h"
#include "elevator_sequence.h"
#include "my_uart.h"
#include "safety_monitor.h"

extern I2C_HandleTypeDef hi2c1;
extern osMutexId_t i2cMutexHandle; //cubeMX에서 Mutex 추가 필요

float g_avg_current = 0.0f; // apMain출력을 위한 공유 변수

static bool g_is_current_sensor_ready = false;
#define MOTOR_TASK_PERIOD_MS                10U
#define OVERCURRENT_RELEASE_COUNT           100U  // 1s(10ms*100) 연속 정상 시 정지 해제
static uint16_t g_normal_current_count = 0;
static uint16_t g_over_current_count = 0;
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
    uint32_t tick_count = osKernelGetTickCount(); 

    while (s_app_ready == false) osDelay(1);

    while (1) {
        // 1. 센서 데이터 취득
        float current = INA219_ReadCurrent_mA(&hi2c1);

        // 2. 안전 감시 (이상 발생 시 내부에서 EmergencyStop 호출)
        Safety_Update(current);

        // 3. 제어기 업데이트 (내부에서 Safety 상태 확인 후 구동 결정)
        Elevator_Controller_Update();

        tick_count += 10U;
        osDelayUntil(tick_count);
    }
}

// function
void apInit(void) {
    bspInit();
    uartInit();
    Elevator_Controller_Init();
    // 전류 센서 초기화 확인
    if (INA219_Init(&hi2c1) != HAL_OK) {
        uartPrintf(0, "INA219 Init Fail!\r\n");
    } else {
        // BSP를 통해 감시 기준값 설정 (예: 500mA)
        bspSetCurrentThreshold(500.0f); 
    }
    
    // Safety 모듈 초기화 (필요 시)
    Safety_Init();

}

void apMain(void) {
    
    //상태보고
    Elevator_ReportStatus();

    if(!Safety_IsSystemSafe()) {
        osDelay(100);
        return;
    }

    //이동 시나리오 실행
    Elevator_RunSequence();

    //다른 기능 추가.

    osDelay(10);
}
