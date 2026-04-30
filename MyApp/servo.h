#ifndef SERVO_H
#define SERVO_H

#include "stm32f1xx_hal.h"
#include "main.h"

typedef enum{
    DOOR_OPEN,
    DOOR_CLOSE
} servo_action_t;

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    servo_action_t state;
    uint16_t min_pulse;
    uint16_t max_pulse;
} servo_t;

void Servo_Init(servo_t *servo, TIM_HandleTypeDef *htim, uint32_t channel);
void Servo_Write(servo_t *servo, uint8_t angle);
void set_servo(servo_t *servo, servo_action_t action);
servo_action_t get_servo(servo_t *servo);

#endif