#include "elevator_controller.h"
#include "Scurve.h"
#include "motor.h"
#include "bsp.h"
#include "motion_unit_conv.h"
#include "pi_controller.h"
#include <math.h>

#define VEL_ZONE_LOW    25.0f  
#define VEL_ZONE_HIGH   40.0f
#define FLOOR_HEIGHT_MM 200.0f 

#define CONTROL_PERIOD_MS   10 
#define CONTROL_DT_SEC      0.01f 

#define MOTOR_MAX_DUTY      4500 
#define MOTOR_MIN_DUTY      700 

#define SCURVE_MAX_VEL_MM_S 150.0f 
#define SCURVE_MAX_ACC_MM_S2 100.0f 
#define SCURVE_JERK_MM_S3   200.0f 

#define FLOOR_DWELL_MS      1000 

#define K_POS               3.0f 

typedef enum {
    ELEVATOR_IDLE,
    ELEVATOR_MOVING,
    ELEVATOR_DWELLING
} ElevatorState_t;

static ElevatorState_t sys_state = ELEVATOR_IDLE;

static uint8_t current_floor = 1;

static float start_pos_mm = 0.0f;
static float target_pos_mm = 0.0f;

static PI_Controller_t my_pi;
static MotionPlanner_t myPlanner;

static uint32_t dwell_timer = 0;
static uint8_t moving_up = 1;

// 속도 계산용
static float last_pos_mm = 0.0f;

void Elevator_Controller_Init() {
    motorClearEncoder();

    start_pos_mm = 0.0f;
    target_pos_mm = 0.0f;

    MP_Init(&myPlanner,
            SCURVE_MAX_VEL_MM_S,
            SCURVE_MAX_ACC_MM_S2,
            SCURVE_JERK_MM_S3,
            CONTROL_DT_SEC);

    MP_SetTarget(&myPlanner, 0.0f);

    PI_Init(&my_pi,
            (float)MOTOR_MAX_DUTY,
            (float)MOTOR_MIN_DUTY,
            VEL_ZONE_LOW,
            VEL_ZONE_HIGH);

    sys_state = ELEVATOR_IDLE;
    current_floor = 1;
}

bool Elevator_GoToFloor(uint8_t target_floor) {
    if (target_floor < 1 || target_floor > 3) return false;
    if (sys_state != ELEVATOR_IDLE) return false;
    if (target_floor == current_floor) return true;

    target_pos_mm = (float)(target_floor - 1) * FLOOR_HEIGHT_MM;
    start_pos_mm = motionPulseToMM((float)motorGetEncoderCount());
    last_pos_mm = start_pos_mm;

    float travel_dist = fabs(target_pos_mm - start_pos_mm);
    moving_up = (target_pos_mm > start_pos_mm) ? 1 : 0;

    uint8_t floor_diff = (target_floor > current_floor) ? 
                         (target_floor - current_floor) : 
                         (current_floor - target_floor);

    float dynamic_max_vel = SCURVE_MAX_VEL_MM_S;
    if (floor_diff == 1) {
        dynamic_max_vel = 90.0f; // 1개 층 이동일 때는 90mm/s 로 제한
    }

    PI_Reset(&my_pi);

    MP_Init(&myPlanner, dynamic_max_vel, SCURVE_MAX_ACC_MM_S2, SCURVE_JERK_MM_S3, CONTROL_DT_SEC);
    MP_SetTarget(&myPlanner, travel_dist);

    current_floor = target_floor;
    sys_state = ELEVATOR_MOVING;

    return true;
}

bool Elevator_IsBusy(void) {
    return (sys_state != ELEVATOR_IDLE);
}

void Elevator_Controller_Update() {

    if (sys_state == ELEVATOR_IDLE) return;

    int32_t enc = motorGetEncoderCount();
    float actual_pos_mm = motionPulseToMM((float)enc);

    // 속도 계산
    float current_vel = (actual_pos_mm - last_pos_mm) / CONTROL_DT_SEC;
    last_pos_mm = actual_pos_mm;

    if (sys_state == ELEVATOR_DWELLING) {
        dwell_timer += CONTROL_PERIOD_MS;

        if (dwell_timer >= FLOOR_DWELL_MS) {
            sys_state = ELEVATOR_IDLE;
            dwell_timer = 0;
        } else {
            motorStop();
        }
        return;
    }

    // S-Curve
    float ideal_travel_dist = MP_Update(&myPlanner);
    float ideal_velocity = myPlanner.velocity;

    float direction = moving_up ? 1.0f : -1.0f;

    float ideal_pos_mm = start_pos_mm + direction * ideal_travel_dist;

    // 위치 루프
    float pos_error = ideal_pos_mm - actual_pos_mm;

    float v_ref_signed = direction * ideal_velocity;

    float target_vel = v_ref_signed + K_POS * pos_error;

    // 속도 제한 (안정성)
    if (target_vel > SCURVE_MAX_VEL_MM_S)
        target_vel = SCURVE_MAX_VEL_MM_S;
    if (target_vel < -SCURVE_MAX_VEL_MM_S)
        target_vel = -SCURVE_MAX_VEL_MM_S;

    // 실제 구동해야 할 방향 결정 (오버슛으로 인해 음수 속도가 나오면 반대로 돌아야 함)
    MotorDir_t cmd_dir = (target_vel >= 0.0f) ? MOTOR_DIR_CW : MOTOR_DIR_CCW;

    //  PI 제어 절댓값처리
    float target_vel_abs = fabs(target_vel);
    float current_vel_abs = fabs(current_vel);

    // PI 제어
    uint32_t duty = (uint32_t)PI_Update(&my_pi,
                                        target_vel_abs,
                                        current_vel_abs,
                                        cmd_dir);

    float final_pwm = duty;

    // 중력 보상 (모터가 실제로 위로 도는 방향일 때만 보상)
    if (cmd_dir == MOTOR_DIR_CW)
        final_pwm += 200;

    if (final_pwm > MOTOR_MAX_DUTY)
        final_pwm = MOTOR_MAX_DUTY;

    if (final_pwm < MOTOR_MIN_DUTY && target_vel_abs > 1.0f)
        final_pwm = MOTOR_MIN_DUTY;

    // 정지 조건
    if (myPlanner.state == MP_IDLE &&
        fabs(pos_error) < 1.0f &&
        fabs(current_vel) < 2.0f) {

        motorStop();
        sys_state = ELEVATOR_DWELLING;
        dwell_timer = 0;
    }
    else {
        motorSetSpeed(cmd_dir, (uint32_t)final_pwm);
    }
}