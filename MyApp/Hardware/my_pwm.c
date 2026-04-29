#include "my_pwm.h"
#include <stdio.h>
#include <stdbool.h>

extern TIM_HandleTypeDef htim4;
static bool s_pwm_started = false;

void myPwmInit(void) {
    // 1. TIM4 채널 3의 PWM 신호 출력을 시작
    if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3) != HAL_OK) {
        s_pwm_started = false;
        printf("[ERR] HAL_TIM_PWM_Start failed (TIM4 CH3)\r\n");
        return;
    }
    s_pwm_started = true;
    
    // 2. 초기 Duty 비율을 0으로 설정하여 모터가 바로 돌지 않도록 방지
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0); 
}

void myPwmSetDuty(uint32_t duty) {
    if (!s_pwm_started) {
        return;
    }

    // ARR를 읽어 현재 타이머 설정에 맞게 안전하게 제한
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
    if (duty > arr) {
        duty = arr;
    }
    
    // 타이머의 CCR(Capture/Compare Register) 값을 갱신하여 Duty 비율을 즉시 변경
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, duty);
}