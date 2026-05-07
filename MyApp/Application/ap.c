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

extern I2C_HandleTypeDef hi2c1;
extern osMutexId_t i2cMutexHandle; //cubeMX에서 Mutex 추가 필요

float g_avg_current = 0.0f; // apMain출력을 위한 공유 변수
bool g_is_system_halted = false;
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

    while (1) {
        // 1. 전류 읽기
        if (g_is_current_sensor_ready) {
            g_avg_current = INA219_ReadCurrent_mA(&hi2c1);
        } else {
            g_avg_current = 0.0f;
        }

        // 2. BSP 로직 (내부에서 2회 연속 감지 시 true 반환)
        if (bspCheckOverCurrent(g_avg_current)) {
            // [정지] 이미 필터링된 결과이므로 즉시 정지
            if (!g_is_system_halted) {
                Elevator_EmergencyStop();
                g_is_system_halted = true;
                g_normal_current_count = 0; // 복구 카운트 초기화
                uartPrintf(0, "!!! STOP: Overcurrent %.1f mA !!!\r\n", g_avg_current);
            }
        } else {
            // [복구] 정상 전류일 때, 정지 상태라면 복구 카운트 진행
            if (g_is_system_halted) {
                if (++g_normal_current_count >= OVERCURRENT_RELEASE_COUNT) {
                    g_is_system_halted = false;
                    g_normal_current_count = 0;
                    uartPrintf(0, "RECOVER: Current normal %.1f mA\r\n", g_avg_current);
                }
            }
        }

        // 3. 모터 제어 업데이트 (정지 상태가 아닐 때만)
        if(!g_is_system_halted) {
            Elevator_Controller_Update();
        }
        
        tick_count += MOTOR_TASK_PERIOD_MS;
        osDelayUntil(tick_count); 
    }
}

// function
void apInit(void) {
    bspInit();
    uartInit();
    Elevator_Controller_Init();
    //전류센서 초기
    g_is_current_sensor_ready = (INA219_Init(&hi2c1) == HAL_OK);
    if (!g_is_current_sensor_ready) {
        uartPrintf(0, "INA219 init fail: check I2C wiring/address\r\n");
    } else {
        bspSetCurrentThreshold(400.0f);
        uartPrintf(0,"Elevator System Ready. Current Monitoring Active.\r\n");
    }

}

void apMain(void) {
    
    //상태보고
    Elevator_ReportStatus();

    if(g_is_system_halted) {
        osDelay(100);
        return;
    }

    //이동 시나리오 실행
    Elevator_RunSequence();

    //다른 기능 추가.

    osDelay(10);
}
