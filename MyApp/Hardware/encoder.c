#include "encoder.h"

extern TIM_HandleTypeDef htim2; // CubeMX 설정에 따라 TIM2 또는 TIM5

void myEncoderInit(void) {
    // 하드웨어 엔코더 카운터 가동
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}

int32_t myEncoderGetCount(void) {
    // 32비트 카운터 값을 반환
    return (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
}

void myEncoderClear(void) {
    // 다음 측정을 위해 카운터를 0으로 초기화
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}