#include "ap.h"
#include "motor.h"
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

    // elevatorInit(&elevator);
}

void apMain(void) {
    // 1. 부팅 후 3초 대기 (MobaXterm을 켜고 준비할 시간을 줍니다)
    printf("System Booting...\r\n");
    printf("Step Response Test will start in 3 seconds...\r\n");
    bspDelay(3000);

    // 2. 실험 시작 알림 및 CSV 헤더 출력
    printf("--- Step Response Test Start ---\r\n");
    printf("Time(ms), EncoderCount\r\n");

    // 3. 하드웨어 초기화 및 100% 전압 인가
    motorClearEncoder();
    motorSetSpeed(MOTOR_DIR_CW, 4500); // ARR이 4500일 때 100% Duty

    uint32_t start_time = bspMillis();
    uint32_t current_time = 0;

    // 4. 2초(2000ms) 동안 10ms 간격으로 캡처
    while (current_time <= 2000) {
        uint32_t now = bspMillis();
        
        if (now - start_time >= current_time) {
            // 현재 시간(ms)과 엔코더 누적 카운트를 출력
            printf("%lu, %ld\r\n", current_time, motorGetEncoderCount());
            current_time += 10; 
        }
    }

    // 5. 2초가 지나면 모터 즉시 정지
    motorStop();
    printf("--- Test End ---\r\n");

    // 6. 실험이 끝났으므로 무한 대기 (더 이상 아무것도 하지 않음)
    while (1) {
        bspDelay(1000);
    }
}