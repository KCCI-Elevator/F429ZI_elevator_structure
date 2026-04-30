#include "elevator_controller.h"
#include "Scurve.h"
#include "motor.h"
#include "Scurve.h"
#include "bsp.h"
#include "motion_unit_conv.h"
#include "def.h"
#include "bsp.h"
#include "elevator_param.h"
#include <stdio.h>
#include <math.h>



/*
 * S-curve 실험용 제어 주기(ms).
 * MP_Init()의 마지막 인자 dt(초)와 반드시 같게 맞출 것 (예: 10ms → 0.01f).
 */
  #if 0
#define CONTROL_PERIOD_MS   10u //10ms마다 한번씩 현재 위치 확인, 다음동작 계산.
#define CONTROL_DT_SEC      ((float)CONTROL_PERIOD_MS / 1000.f)//위의 값을 초단위로 환산.
#define MOTOR_MAX_DUTY      3000u //최대 pwm 설정값
#define MOTOR_MIN_DUTY      700u // 최소 pwm 설정값 데드존 넘어서는 값 설정.
#define SCURVE_TRAVEL_MM    400.0f //이동 목표 거리.(20cm로 설정.)
#define SCURVE_MAX_VEL_MM_S 50.0f //최대속도
#define SCURVE_MAX_ACC_MM_S2 35.0f //최대 가속도
#define SCURVE_JERK_MM_S3   25.0f //가속도 변화율(jerk)
#define FLOOR_DWELL_MS      10u //도착 후 대기 시간.(300ms)

#endif

 #if 0
#define CONTROL_PERIOD_MS   10u //10ms마다 한번씩 현재 위치 확인, 다음동작 계산.
#define CONTROL_DT_SEC      ((float)CONTROL_PERIOD_MS / 1000.f)//위의 값을 초단위로 환산.
#define MOTOR_MAX_DUTY      4000u //최대 pwm 설정값
#define MOTOR_MIN_DUTY      700u // 최소 pwm 설정값 데드존 넘어서는 값 설정.
#define SCURVE_TRAVEL_MM    520.0f //이동 목표 거리.(52cm로 설정.)
#define SCURVE_MAX_VEL_MM_S 15.0f //최대속도
#define SCURVE_MAX_ACC_MM_S2 4.7f //최대 가속도
#define SCURVE_JERK_MM_S3   0.9f //가속도 변화율(jerk)
#define FLOOR_DWELL_MS      300u //도착 후 대기 시간.(300ms)

#endif //52cm

static MotionPlanner_t myPlanner;
static float s_target_mm = 0.0f;



void Elevator_Controller_Init() {
    motorClearEncoder();
    MP_Init(&myPlanner, SCURVE_MAX_VEL_MM_S, SCURVE_MAX_ACC_MM_S2, SCURVE_JERK_MM_S3, CONTROL_DT_SEC);
    

}

void Elevator_Controller_SetTarget(float distance_mm) {
    motorClearEncoder();//이동시작 전 엔코더 리셋
     MP_Init(&myPlanner, SCURVE_MAX_VEL_MM_S, SCURVE_MAX_ACC_MM_S2, SCURVE_JERK_MM_S3, CONTROL_DT_SEC);
     MP_SetTarget(&myPlanner, distance_mm);
     s_target_mm = distance_mm;

}

bool Elevator_Controller_Update(uint8_t direction) {
     int32_t enc = motorGetEncoderCount();
        int32_t enc_abs = (int32_t)labs(enc);//절댓값
        float enc_mm = motionPulseToMM((float)enc_abs);

        // 실제 엔코더 기준 도착 판정(프로파일 상태와 무관하게 확실히 정지)
        if (enc_mm >= (s_target_mm - 1.0f)) {
            motorStop();
            return true;
        }

        //scuve 궤적 계산
        MP_Update(&myPlanner);
        float vel_ratio = myPlanner.velocity / SCURVE_MAX_VEL_MM_S;
            if (vel_ratio < 0.0f) vel_ratio = 0.0f;
            if (vel_ratio > 1.0f) vel_ratio = 1.0f;

            uint32_t current_duty = MOTOR_MIN_DUTY + (uint32_t)((MOTOR_MAX_DUTY - MOTOR_MIN_DUTY) * vel_ratio);
           

            if(myPlanner.velocity < 0.5f) current_duty = MOTOR_MIN_DUTY;

            //도착 판정
            if(myPlanner.state == MP_DONE) {
                motorStop();
                return true;//도착
            }
            motorSetSpeed(direction == 1 ? MOTOR_DIR_CW : MOTOR_DIR_CCW, current_duty);
            return false;//아직 가는중

           
        }
      



