#include "servo.h"

void Servo_Init(servo_t *servo, TIM_HandleTypeDef *htim, uint32_t channel)
{
    servo->htim = htim;
    servo->channel = channel;
    servo->state = DOOR_CLOSE;
    servo->min_pulse = 1260;
    servo->max_pulse = 2190;
    HAL_TIM_PWM_Start(servo->htim, servo->channel);
    Servo_Write(servo, 90);
}

void Servo_Write(servo_t *servo, uint8_t angle)
{
    if (angle > 180) angle = 180;
    uint16_t pulse = servo->min_pulse +
    ((uint32_t)angle * (servo->max_pulse - servo->min_pulse)) / 180;
    __HAL_TIM_SET_COMPARE(servo->htim, servo->channel, pulse);
}

void set_servo(servo_t *servo, servo_action_t action)
{
    servo->state = action;
    if (action == DOOR_OPEN) Servo_Write(servo, 180);
    else Servo_Write(servo, 0);
}

servo_action_t get_servo(servo_t *servo)
{
    return servo->state;
}