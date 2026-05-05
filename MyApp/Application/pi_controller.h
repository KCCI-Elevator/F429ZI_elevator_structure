#ifndef __PI_CONTROLLER_H__
#define __PI_CONTROLLER_H__

#include "def.h"
#include "motor.h"

// 1. 50% PWM 구간 게인 (저속)
#define PI_UP_KP_50    33.673f
#define PI_UP_KI_50    652.926f
#define PI_DN_KP_50    20.473f
#define PI_DN_KI_50    239.769f

// 2. 75% PWM 구간 게인 (중속)
#define PI_UP_KP_75    22.467f
#define PI_UP_KI_75    557.226f
#define PI_DN_KP_75    18.4459f
#define PI_DN_KI_75    375.447f

// 3. 100% PWM 구간 게인 (고속)
#define PI_UP_KP_100   23.483f
#define PI_UP_KI_100   1449.541f
#define PI_DN_KP_100   21.574f
#define PI_DN_KI_100   1248.798f

// PI 제어기 구조체
typedef struct {
    // 사용자 입력 게인 (상승/하강)
    float Kp_up[3];
    float Ki_up[3];
    float Kp_down[3];
    float Ki_down[3];

    // 구간 판단용 속도 임계값
    float vel_zone_low;
    float vel_zone_high;
    
    // 내부 상태 및 제한 변수
    float integral_sum;  // I항
    float max_pwm;       
    float min_pwm;       // 최소 기동 PWM
    float dt;            // 제어 주기 (10ms)
} PI_Controller_t;

void PI_Init(PI_Controller_t *pi, float max_out, float min_out, float v_low, float v_high);
float PI_Update(PI_Controller_t *pi, float target_vel, float current_vel, MotorDir_t dir);
void PI_Reset(PI_Controller_t *pi);

#endif // __PI_CONTROLLER_H__