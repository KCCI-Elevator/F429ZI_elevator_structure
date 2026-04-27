#include "ap.h"
#include "motor.h"
#include <stdio.h>

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
    bspInit(); // 내부에서 motorInit()이 호출되어 클럭 및 GPIO가 초기화되어야 합니다.
}

void apMain(void) {
    // 1. 부팅 및 준비 대기
    printf("\r\n--- Full System Identification Start ---\r\n");
    printf("Wait 3 seconds for MobaXterm logging...\r\n");
    bspDelay(3000);

    // CSV 헤더 출력 (구분자 포함)
    printf("Phase, Time(ms), EncoderCount\r\n");

    uint32_t start_time;
    uint32_t current_time;

    // ==========================================
    // 2. CW (정방향) 가속 테스트 (100% PWM, 1초)
    // ==========================================
    motorClearEncoder();
    motorSetSpeed(MOTOR_DIR_CW, 4500); 
    start_time = bspMillis();
    current_time = 0;

    while (current_time <= 1000) {
        uint32_t now = bspMillis();
        if (now - start_time >= current_time) {
            printf("CW_ACC, %lu, %ld\r\n", current_time, motorGetEncoderCount());
            current_time += 10; 
        }
    }

    // ==========================================
    // 3. CW (정방향) 감속/정지 테스트 (0% PWM, 0.5초)
    // ==========================================
    motorStop(); // 전압 차단 (마찰력과 관성으로만 멈춤)
    start_time = bspMillis(); // 시간 축을 0부터 다시 계산 (분석 용이성)
    current_time = 0;

    while (current_time <= 500) {
        uint32_t now = bspMillis();
        if (now - start_time >= current_time) {
            printf("CW_STOP, %lu, %ld\r\n", current_time, motorGetEncoderCount());
            current_time += 10; 
        }
    }

    bspDelay(1000); // 방향 전환 전 기구적 안정화 대기

    // ==========================================
    // 4. CCW (역방향) 가속 테스트 (100% PWM, 1초)
    // ==========================================
    motorClearEncoder(); // 역방향 측정을 위해 0으로 다시 초기화
    motorSetSpeed(MOTOR_DIR_CCW, 4500); 
    start_time = bspMillis();
    current_time = 0;

    while (current_time <= 1000) {
        uint32_t now = bspMillis();
        if (now - start_time >= current_time) {
            printf("CCW_ACC, %lu, %ld\r\n", current_time, motorGetEncoderCount());
            current_time += 10; 
        }
    }

    // ==========================================
    // 5. CCW (역방향) 감속/정지 테스트 (0% PWM, 0.5초)
    // ==========================================
    motorStop();
    start_time = bspMillis();
    current_time = 0;

    while (current_time <= 500) {
        uint32_t now = bspMillis();
        if (now - start_time >= current_time) {
            printf("CCW_STOP, %lu, %ld\r\n", current_time, motorGetEncoderCount());
            current_time += 10; 
        }
    }

    printf("--- All Tests Completed ---\r\n");

    // 6. 무한 루프 (재실행 방지)
    while (1) {
        bspDelay(1000);
    }
}