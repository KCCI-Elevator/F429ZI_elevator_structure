#include "my_pwm.h"

extern TIM_HandleTypeDef htim4;

void myPwmInit(void) {
    // 1. TIM4 채널 3의 PWM 신호 출력을 시작
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    
    // 2. 초기 Duty 비율을 0으로 설정하여 모터가 바로 돌지 않도록 방지
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0); 
}

void myPwmSetDuty(uint32_t duty) {
    // ARR 값이 4499이므로 4500 이상의 값이 CCR 레지스터에 들어가면 
    // PWM 핀이 계속 High 상태로 멈춰버리는 문제가 발생할 수 있습니다.
    if (duty > 4500) {
        duty = 4500; 
    }
    
    // 타이머의 CCR(Capture/Compare Register) 값을 갱신하여 Duty 비율을 즉시 변경
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, duty);
}