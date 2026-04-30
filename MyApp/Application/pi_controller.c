#include "pi_controller.h"
#include "motor.h"

void PI_Init(PI_Controller_t *pi, float max_out, float min_out, float v_low, float v_high) {
    // 50%
    pi->Kp_up[0] = PI_UP_KP_50;  pi->Ki_up[0] = PI_UP_KI_50;
    pi->Kp_down[0] = PI_DN_KP_50; pi->Ki_down[0] = PI_DN_KI_50;
    
    // 75%
    pi->Kp_up[1] = PI_UP_KP_75;  pi->Ki_up[1] = PI_UP_KI_75;
    pi->Kp_down[1] = PI_DN_KP_75; pi->Ki_down[1] = PI_DN_KI_75;
    
    // 100%
    pi->Kp_up[2] = PI_UP_KP_100; pi->Ki_up[2] = PI_UP_KI_100;
    pi->Kp_down[2] = PI_DN_KP_100; pi->Ki_down[2] = PI_DN_KI_100;

    pi->vel_zone_low = v_low;
    pi->vel_zone_high = v_high;
    pi->max_pwm = max_out;
    pi->min_pwm = min_out;
    pi->dt = 0.01f;
    pi->integral_sum = 0.0f;
}

float PI_Update(PI_Controller_t *pi, float target_vel, float current_vel, MotorDir_t dir) {
    float error = target_vel - current_vel;
    float kp, ki;
    uint8_t zone = 0;

    if (target_vel >= pi->vel_zone_high) {
        zone = 2; // 100% 구간
    } else if (target_vel >= pi->vel_zone_low) {
        zone = 1; // 75% 구간
    } else {
        zone = 0; // 50% 구간
    }

    if (dir == MOTOR_DIR_CW) {
        kp = pi->Kp_up[zone];
        ki = pi->Ki_up[zone];
    } else {
        kp = pi->Kp_down[zone];
        ki = pi->Ki_down[zone];
    }

    // p항
    float p_term = kp * error;

    // i항
    pi->integral_sum += error * pi->dt;
    float i_term = ki * pi->integral_sum;

    // U(PID) = Kp*E + Ki*E + Kd*E;
    float output = p_term + i_term;

    // 정지 시 초기화
    if (target_vel <= 0.0f) {
        pi->integral_sum = 0.0f;
        return 0.0f;
    }

    // Feedforward
    output += pi->min_pwm;

    // Anti-Windup
    if (output > pi->max_pwm) {
        output = pi->max_pwm;
        if (error > 0.0f) {
            pi->integral_sum -= error * pi->dt;
        }
    } else if (output < pi->min_pwm) {
        output = pi->min_pwm;
        if (error < 0.0f) {
            pi->integral_sum -= error * pi->dt;
        }
    }

    return output;

}

void PI_Reset(PI_Controller_t *pi) {
    pi->integral_sum = 0.0f;
}