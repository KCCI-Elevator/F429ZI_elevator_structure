#include "elevator_controller.h"
#include "Scurve.h"
#include "motor.h"
#include "Scurve.h"
#include "bsp.h"
#include "motion_unit_conv.h"
#include "def.h"
#include "pi_controller.h"


#define VEL_ZONE_LOW    25.0f  // 25mm/s 미만은 50% 구간 게인 사용
#define VEL_ZONE_HIGH   40.0f

/*
 * S-curve 실험용 제어 주기(ms).
 * MP_Init()의 마지막 인자 dt(초)와 반드시 같게 맞출 것 (예: 10ms → 0.01f).
 */
  #if 1
#define CONTROL_PERIOD_MS   10u //10ms마다 한번씩 현재 위치 확인, 다음동작 계산.
#define CONTROL_DT_SEC      ((float)CONTROL_PERIOD_MS / 1000.f)//위의 값을 초단위로 환산.
#define MOTOR_MAX_DUTY      3000u //최대 pwm 설정값
#define MOTOR_MIN_DUTY      700u // 최소 pwm 설정값 데드존 넘어서는 값 설정.
#define SCURVE_TRAVEL_MM    400.0f //이동 목표 거리.(20cm로 설정.)
#define SCURVE_MAX_VEL_MM_S 150.0f //최대속도
#define SCURVE_MAX_ACC_MM_S2 100.0f //최대 가속도
#define SCURVE_JERK_MM_S3   200.0f //가속도 변화율(jerk)
#define FLOOR_DWELL_MS      1000u //도착 후 대기 시간

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

static PI_Controller_t my_pi;    
static float last_enc_mm = 0.0f;

static MotionPlanner_t myPlanner;
static uint8_t moving_up = 1; // 1: 상행(CW), 0: 하행(CCW)
static uint32_t elapsed_ms = 0u;
static uint32_t dwell_timer = 0; //대기시간측정 변수

void Elevator_Controller_Init() {
    motorClearEncoder();
    MP_Init(&myPlanner, SCURVE_MAX_VEL_MM_S, SCURVE_MAX_ACC_MM_S2, SCURVE_JERK_MM_S3, CONTROL_DT_SEC);
    MP_SetTarget(&myPlanner, SCURVE_TRAVEL_MM);

    PI_Init(&my_pi, (float)MOTOR_MAX_DUTY, (float)MOTOR_MIN_DUTY, VEL_ZONE_LOW, VEL_ZONE_HIGH);

    last_enc_mm = 0.0f;
    printf("time_ms,target_mm,target_vel_mm_s,duty,dir,enc_pulse,enc_mm\r\n");

}

void Elevator_Controller_Update() {
    int32_t enc = motorGetEncoderCount();
    int32_t enc_abs = (int32_t)labs(enc); // 절댓값
    float enc_mm = motionPulseToMM((float)enc_abs);

    // 1. 대기 중 상태 확인 및 방향 전환 전 완벽한 초기화
    if(dwell_timer > 0) {
        dwell_timer += CONTROL_PERIOD_MS;
        if(dwell_timer >= FLOOR_DWELL_MS) {
            dwell_timer = 0; // 대기 종료
            
            // 관성으로 완전히 멈춘 상태에서 모든 것을 초기화
            motorClearEncoder();
            last_enc_mm = 0.0f;
            PI_Reset(&my_pi);
            MP_Init(&myPlanner, SCURVE_MAX_VEL_MM_S, SCURVE_MAX_ACC_MM_S2, SCURVE_JERK_MM_S3, CONTROL_DT_SEC);
            MP_SetTarget(&myPlanner, SCURVE_TRAVEL_MM);
            elapsed_ms = 0;

            printf("--- Direction Changed: %s ---\r\n", moving_up ? "UP" : "DOWN");
            
            // 현재 루프의 측정값도 0으로 갱신
            enc = 0; enc_abs = 0; enc_mm = 0.0f; 
        } else {
            motorStop(); // 대기 중에는 계속 정지 명령
            return;
        }
    }

    float target_mm = MP_Update(&myPlanner);
    float enc_signed_mm = motionPulseToMM((float)enc);

    // 🎯 [핵심 로직 통합]
    // 구동 조건: 실제 거리가 아직 모자라거나(enc_mm < 400.0), S-Curve가 아직 진행 중일 때
    if (enc_mm < SCURVE_TRAVEL_MM || myPlanner.state != MP_IDLE) {
        
        float target_vel = myPlanner.velocity;
        
        // 🛠️ [수정안 2 반영] S-Curve가 끝났는데(DONE 또는 IDLE) 거리가 모자라면 크립 주행 유지!
        if ((myPlanner.state == MP_DONE || myPlanner.state == MP_IDLE) && enc_mm < SCURVE_TRAVEL_MM) {
            target_vel = 10.0f; // 남은 거리를 10mm/s로 꾸역꾸역 밀어 올림
        }

        // 🛠️ [수정안 1 반영] 하강 시 오버슛 방어: 거리는 400.0을 넘었는데 S-Curve가 아직 안 끝났다면 브레이크!
        if (enc_mm >= SCURVE_TRAVEL_MM && myPlanner.state != MP_IDLE) {
             target_vel = 0.0f; // 강제 제동 명령
        }

        // 모터 현재 속도 계산
        float current_vel = fabs((enc_mm - last_enc_mm) / CONTROL_DT_SEC);
        last_enc_mm = enc_mm;

        // 이동 방향 판별
        MotorDir_t current_dir = moving_up ? MOTOR_DIR_CW : MOTOR_DIR_CCW;

        // PI Controller (강력한 새 게인 적용됨)
        uint32_t current_duty = (uint32_t)PI_Update(&my_pi, target_vel, current_vel, current_dir);

        // Deadband logic
        if (target_vel < 0.5f) {
            current_duty = 0;
        } else if (current_duty < MOTOR_MIN_DUTY) {
            current_duty = MOTOR_MIN_DUTY;
        }
        motorSetSpeed(current_dir, current_duty);

        // 현재 상태 출력 (디버깅용)
        
        printf("%lu,%.2f,%.2f,%lu,%s,%ld,%.2f\r\n",
               (unsigned long)elapsed_ms, target_mm, myPlanner.velocity,
               (unsigned long)current_duty, moving_up ? "UP" : "DOWN",
               (long)enc, enc_signed_mm);
        

    } else {
        // 🎯 도달 완료 로직: 거리가 400.0 이상이고, S-Curve도 IDLE 상태일 때만 진입
        motorStop();
        if(dwell_timer == 0) {
            dwell_timer = 1; // 대기 시작 플래그 (1ms부터 카운트)
            moving_up = moving_up ? 0 : 1; // 방향 전환 예약
        }
    }
    elapsed_ms += CONTROL_PERIOD_MS;
}

// void Elevator_Controller_Update() {
//      int32_t enc = motorGetEncoderCount();
//         int32_t enc_abs = (int32_t)labs(enc); //절댓값
//         // S-curve 로직의 거리 판단을 실제 엔코더 진행량 기준으로 동기화
//         float enc_mm = motionPulseToMM((float)enc_abs);
       

//         //대기 중 상태 확인
//         if(dwell_timer> 0) {
//             dwell_timer += CONTROL_PERIOD_MS;
//             if(dwell_timer >= FLOOR_DWELL_MS) {
//                 dwell_timer = 0; 
//             } else {
//                 motorStop(); //대기중에는 계속 정지 명령
//                 return;
//             }
//         }

//         float target_mm = MP_Update(&myPlanner); // scurve값 가져오기.
//         float enc_signed_mm = motionPulseToMM((float)enc);

//         //상태가 done이 아니고 아직 목표거리에 도달하지 않았을 때만 구동
//         if (enc_mm < SCURVE_TRAVEL_MM) {
            
//             // S-Curve 알고리즘의 출력인 10ms 샘플링 목표 속도
//             float target_vel = myPlanner.velocity;
            
//             if (myPlanner.state == MP_DONE) {
//                 target_vel = 10.0f; // 10mm/s 
//             }

//             // 모터 현재 속도 계산
//             float current_vel = fabs((enc_mm - last_enc_mm) / CONTROL_DT_SEC);
//             last_enc_mm = enc_mm;

//             // 이동 방향 판별
//             MotorDir_t current_dir = moving_up ? MOTOR_DIR_CW : MOTOR_DIR_CCW;

//             // PI Controller
//             uint32_t current_duty = (uint32_t)PI_Update(&my_pi, target_vel, current_vel, current_dir);;

//             // Deadband logic
//             if (target_vel < 0.5f) {
//                 current_duty = 0;
//             } else if (current_duty < MOTOR_MIN_DUTY) {
//                 current_duty = MOTOR_MIN_DUTY;
//             }
//             motorSetSpeed(current_dir, current_duty);

//             //현재 상태 출력
//             printf("%lu,%.2f,%.2f,%lu,%s,%ld,%.2f\r\n",
//                    (unsigned long)elapsed_ms,
//                    target_mm,
//                    myPlanner.velocity,
//                    (unsigned long)current_duty,
//                    moving_up ? "UP" : "DOWN",
//                    (long)enc,
//                    enc_signed_mm);

//         } else {
//             motorStop();
//             if(dwell_timer == 0) {
//                 dwell_timer = 1; //대기 시작 플래그 (1ms부터 카운트)

//                 //방향 전환 및 초기화.
//                 moving_up = moving_up ? 0 : 1;
//                 elapsed_ms = 0;
//                 motorClearEncoder();

//                 last_enc_mm = 0.0f; 
//                 PI_Reset(&my_pi);

//                 MP_Init(&myPlanner, SCURVE_MAX_VEL_MM_S, SCURVE_MAX_ACC_MM_S2, SCURVE_JERK_MM_S3, CONTROL_DT_SEC);
//                 MP_SetTarget(&myPlanner, SCURVE_TRAVEL_MM);

//                 printf("--- Direction Changed: %s ---\r\n", moving_up ? "UP" : "DOWN");
//                 printf("time_ms,target_mm,target_vel_mm_s,duty,dir,enc_pulse,enc_mm\r\n");
//             }

           
//         }
//         elapsed_ms+=CONTROL_PERIOD_MS;

// }

