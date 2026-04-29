#include "motor.h"
#include "my_pwm.h"
#include "encoder.h"
#include <stdio.h>

void motorInit(void) {
    // 🚨 1. 하드웨어 클럭(전원) 강제 ON 
    __HAL_RCC_GPIOB_CLK_ENABLE(); // 내장 LED용 (PB0, PB14)
    __HAL_RCC_GPIOF_CLK_ENABLE(); // 모터 제어용 (PF14, PF15)

    // 2. 내장 LED 핀 (PB0, PB14) 초기화 설정
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 3. 모터 제어 핀 (PF14, PF15) 초기화 설정
    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    // 4. 초기 상태: LED 끄기 및 모터 정지 상태로 고정
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

    // 5. 타이머(PWM, 엔코더) 하드웨어 초기화
    myPwmInit();
    myEncoderInit();
}

void motorSetSpeed(MotorDir_t dir, uint32_t duty) {
    if (dir == MOTOR_DIR_CW) {
        // 정방향: 초록 LED(PB0) 켜기 + 모터 PF14 켜기
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); 
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);    
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_14, GPIO_PIN_SET);
    } 
    else if (dir == MOTOR_DIR_CCW) {
        // 역방향: 빨간 LED(PB14) 켜기 + 모터 PF15 켜기
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_14, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_15, GPIO_PIN_SET);
    }
    else {
        motorStop();
        return;
    }
    myPwmSetDuty(duty);
}

void motorStop(void) {
    // 정지: 두 LED 모두 끄기 + 모터 방향 핀 둘 다 LOW
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_15, GPIO_PIN_RESET);
    myPwmSetDuty(0);
}

void motorDebugDump(void) {
    extern TIM_HandleTypeDef htim4;
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
    uint32_t ccr = __HAL_TIM_GET_COMPARE(&htim4, TIM_CHANNEL_3);
    GPIO_PinState in1 = HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_14);
    GPIO_PinState in2 = HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_15);
    printf("[MOTOR] IN1=%u IN2=%u PWM=%lu/%lu\r\n",
           (unsigned)in1, (unsigned)in2, (unsigned long)ccr, (unsigned long)arr);
}

int32_t motorGetEncoderCount(void) {
    return myEncoderGetCount();
}

void motorClearEncoder(void) {
    myEncoderClear();
}