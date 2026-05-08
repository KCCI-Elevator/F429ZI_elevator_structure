#include "motor.h"
#include "my_pwm.h"
#include "my_encoder.h"
#include "my_gpio.h"

// 포트/핀 매핑 (0=A, 1=B, 5=F)
// 모터 제어용 포트 및 핀 정의 (PF14, PF15)
#define MOT_PORT 5      // PF
#define MOT_CW_PIN  14  // PF14
#define MOT_CCW_PIN 15  // PF15

void motorInit(void) {
    // 1. gpio initialize
    gpioExtInit(MOT_PORT, MOT_CW_PIN, GPIO_MODE_OUTPUT_PP);
    gpioExtInit(MOT_PORT, MOT_CCW_PIN, GPIO_MODE_OUTPUT_PP);

    // 2. initialize
    motorStop();
}

void motorSetSpeed(MotorDir_t dir, uint32_t duty) {
    if (dir == MOTOR_DIR_CW) {
        gpioExtWrite(MOT_PORT, MOT_CCW_PIN, 0);
        gpioExtWrite(MOT_PORT, MOT_CW_PIN, 1);
    } 
    else if (dir == MOTOR_DIR_CCW) {
        gpioExtWrite(MOT_PORT, MOT_CW_PIN, 0);
        gpioExtWrite(MOT_PORT, MOT_CCW_PIN, 1);
    }
    else {
        motorStop();
        return;
    }
    
    myPwmSetDuty(duty); 
}

void motorStop(void) {
    // 방향 제어 핀 모두 LOW 설정
    gpioExtWrite(MOT_PORT, MOT_CW_PIN, 0);
    gpioExtWrite(MOT_PORT, MOT_CCW_PIN, 0);
    myPwmSetDuty(0);
}

int32_t motorGetEncoderCount(void) {
    return myEncoderGetCount();
}

void motorClearEncoder(void) {
    myEncoderClear();
}