#ifndef __MAP_DRIVER__MOTOR_H__
#define __MAP_DRIVER__MOTOR_H__

#include "def.h"

// 방향 열거형 선언
typedef enum {
    MOTOR_DIR_STOP = 0,
    MOTOR_DIR_CW,
    MOTOR_DIR_CCW
} MotorDir_t;

void motorInit(void);
void motorSetSpeed(MotorDir_t dir, uint32_t duty);
void motorStop(void);

int32_t motorGetEncoderCount(void);
void motorClearEncoder(void);

#endif //__MAP_DRIVER__MOTOR_H__