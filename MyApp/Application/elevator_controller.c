#include "elevator_controller.h"
#include "Scurve.h"
#include "motor.h"
#include "Scurve.h"
#include "bsp.h"
#include "motion_unit_conv.h"
#include "def.h"
#include <stdio.h>
#include <math.h>


/*
 * S-curve 실험용 제어 주기(ms).
 * MP_Init()의 마지막 인자 dt(초)와 반드시 같게 맞출 것 (예: 10ms → 0.01f).
 */
  #if 1
#define CONTROL_PERIOD_MS   10u //10ms마다 한번씩 현재 위치 확인, 다음동작 계산.
#define CONTROL_DT_SEC      ((float)CONTROL_PERIOD_MS / 1000.f)//위의 값을 초단위로 환산.
#define MOTOR_MAX_DUTY      4000u //최대 pwm 설정값
#define MOTOR_MIN_DUTY      700u // 최소 pwm 설정값 데드존 넘어서는 값 설정.
#define SCURVE_TRAVEL_MM    100.0f //이동 목표 거리.(20cm로 설정.)
#define SCURVE_MAX_VEL_MM_S 40.0f //최대속도
#define SCURVE_MAX_ACC_MM_S2 20.0f //최대 가속도
#define SCURVE_JERK_MM_S3   10.0f //가속도 변화율(jerk)
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
static uint8_t moving_up = 1u; // 1: 상행(CW), 0: 하행(CCW)
static uint32_t elapsed_ms = 0u;
static uint32_t dwell_timer = 0; //대기시간측정 변수

void Elevator_Controller_Init() {
    motorClearEncoder();
    MP_Init(&myPlanner, SCURVE_MAX_VEL_MM_S, SCURVE_MAX_ACC_MM_S2, SCURVE_JERK_MM_S3, CONTROL_DT_SEC);
    MP_SetTarget(&myPlanner, SCURVE_TRAVEL_MM);
    printf("time_ms,target_mm,target_vel_mm_s,duty,dir,enc_pulse,enc_mm\r\n");

}

void Elevator_Controller_Update() {
     int32_t enc = motorGetEncoderCount();
        int32_t enc_abs = (int32_t)labs(enc);//절댓값
        // S-curve 로직의 거리 판단을 실제 엔코더 진행량 기준으로 동기화
        float enc_mm = motionPulseToMM((float)enc_abs);
        myPlanner.current_pos = enc_mm;

        //대기중인 상태인지 먼저 확인
        if(dwell_timer> 0) {
            dwell_timer += CONTROL_PERIOD_MS;
            if(dwell_timer >= FLOOR_DWELL_MS) {
                dwell_timer = 0; //대기 종료
            } else {
                motorStop(); //대기중에는 계속 정지 명령
                return; //함수 종료 (지연 없이 바로 리턴)
            }
        }

        float target_mm = MP_Update(&myPlanner);//scurve값 가져오기.
        float enc_signed_mm = motionPulseToMM((float)enc);

        //상태가 done이 아니고 아직 목표거리에 도달하지 않았을 때만 구동
        if (myPlanner.state != MP_IDLE && myPlanner.state != MP_DONE && enc_mm < SCURVE_TRAVEL_MM) {
            //속도/최대속도 지금 속도비율로 pwm 계산.
            float vel_ratio = myPlanner.velocity / SCURVE_MAX_VEL_MM_S;
            if (vel_ratio < 0.0f) vel_ratio = 0.0f;
            if (vel_ratio > 1.0f) vel_ratio = 1.0f;
            uint32_t current_duty = MOTOR_MIN_DUTY + (uint32_t)((MOTOR_MAX_DUTY - MOTOR_MIN_DUTY) * vel_ratio);
            if (current_duty < MOTOR_MIN_DUTY) current_duty = MOTOR_MIN_DUTY;
            if (current_duty > MOTOR_MAX_DUTY) current_duty = MOTOR_MAX_DUTY;

            if(myPlanner.velocity < 0.5f) current_duty = 0;
            motorSetSpeed(moving_up ? MOTOR_DIR_CW : MOTOR_DIR_CCW, current_duty);

            //현재 상태 출력
            printf("%lu,%.2f,%.2f,%lu,%s,%ld,%.2f\r\n",
                   (unsigned long)elapsed_ms,
                   target_mm,
                   myPlanner.velocity,
                   (unsigned long)current_duty,
                   moving_up ? "UP" : "DOWN",
                   (long)enc,
                   enc_signed_mm);
        } else {
            motorStop();
            if(dwell_timer == 0) {
                dwell_timer = 1; //대기 시작 플래그 (1ms부터 카운트)

                //방향 전환 및 초기화.
                moving_up = moving_up ? 0u : 1u;
                elapsed_ms = 0u;
                motorClearEncoder();

                MP_Init(&myPlanner, SCURVE_MAX_VEL_MM_S, SCURVE_MAX_ACC_MM_S2, SCURVE_JERK_MM_S3, CONTROL_DT_SEC);
                MP_SetTarget(&myPlanner, SCURVE_TRAVEL_MM);

                printf("--- Direction Changed: %s ---\r\n", moving_up ? "UP" : "DOWN");
                printf("time_ms,target_mm,target_vel_mm_s,duty,dir,enc_pulse,enc_mm\r\n");
            }

           
        }
        elapsed_ms+=CONTROL_PERIOD_MS;

}

