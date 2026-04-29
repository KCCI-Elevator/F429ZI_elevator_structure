#include "ap.h"
#include "motor.h"
#include <stdio.h>
#include "cmsis_os2.h" // RTOS API 사용을 위해 포함

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

void apInit(void) {
    bspInit(); 
}

void apMain(void) {
    // 식별(ID) 테스트는 부팅 후 1회만 수행되도록 플래그 처리
    static bool is_test_done = false;
    
    if (!is_test_done) {
        printf("\r\n--- Full System Identification Start ---\r\n");
        printf("Wait 3 seconds for MobaXterm logging...\r\n");
        osDelay(3000);

        printf("Phase, Time(ms), EncoderCount\r\n");

        uint32_t start_time;
        uint32_t current_time;

        // ==========================================
        // 1. CW (정방향) 가속 테스트 (2초)
        // ==========================================
        motorClearEncoder();
        motorSetSpeed(MOTOR_DIR_CW, 4500); 
        start_time = osKernelGetTickCount();
        current_time = 0;

        while (current_time <= 2000) {
            uint32_t now = osKernelGetTickCount();
            if (now - start_time >= current_time) {
                printf("CW_ACC, %lu, %ld\r\n", current_time, motorGetEncoderCount());
                current_time += 10; 
            }
            osDelay(1);
        }

        // ==========================================
        // 2. CW (정방향) 감속/정지 테스트 (0.5초)
        // ==========================================
        motorStop();
        start_time = osKernelGetTickCount();
        current_time = 0;

        while (current_time <= 1000) {
            uint32_t now = osKernelGetTickCount();
            if (now - start_time >= current_time) {
                printf("CW_STOP, %lu, %ld\r\n", current_time, motorGetEncoderCount());
                current_time += 10; 
            }
            osDelay(1);
        }

        osDelay(1000); // 방향 전환 전 안정화 대기

        // ==========================================
        // 3. CCW (역방향) 가속 테스트 (2초)
        // ==========================================
        motorClearEncoder();
        motorSetSpeed(MOTOR_DIR_CCW, 4500); 
        start_time = osKernelGetTickCount();
        current_time = 0;

        while (current_time <= 2000) {
            uint32_t now = osKernelGetTickCount();
            if (now - start_time >= current_time) {
                printf("CCW_ACC, %lu, %ld\r\n", current_time, motorGetEncoderCount());
                current_time += 10; 
            }
            osDelay(1);
        }

        // ==========================================
        // 4. CCW (역방향) 감속/정지 테스트 (0.5초)
        // ==========================================
        motorStop();
        start_time = osKernelGetTickCount();
        current_time = 0;

        while (current_time <= 1000) {
            uint32_t now = osKernelGetTickCount();
            if (now - start_time >= current_time) {
                printf("CCW_STOP, %lu, %ld\r\n", current_time, motorGetEncoderCount());
                current_time += 10; 
            }
            osDelay(1);
        }

        printf("--- All Tests Completed ---\r\n");
        is_test_done = true; // 플래그를 세워 다음 루프부터는 실행되지 않도록 함
    }

    osDelay(100); 
}