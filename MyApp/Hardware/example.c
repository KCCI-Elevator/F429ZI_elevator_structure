
// 기존 예제
#if 0
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    HAL_ADC_Start(&hadc1);

    /* 정방향 */
    HAL_GPIO_WritePin(MOTOR_IN1_GPIO_Port, MOTOR_IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_IN2_GPIO_Port, MOTOR_IN2_Pin, GPIO_PIN_RESET);
#endif

#if 0
    HAL_ADC_PollForConversion(&hadc1, 10);

    uint32_t adc_value = HAL_ADC_GetValue(&hadc1);   // 0 ~ 4095
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
    uint32_t duty = (adc_value * arr) / 4095;

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, duty);

    HAL_Delay(10);
    
#endif