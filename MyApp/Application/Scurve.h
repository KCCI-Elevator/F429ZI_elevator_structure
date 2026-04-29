#ifndef SCURVE_H_
#define SCURVE_H_

#include "def.h"

typedef enum {
    MP_IDLE = 0,
    MP_ACCEL_INC, //가속도 증가 구간
    MP_ACCEL_CONST,
    MP_ACCEL_DEC,// 최대 가속도 도달 후 감속.
    MP_CRUISE, //정속 주행
    MP_DECEL_INC, //감속 시작
    MP_DECEL_CONST,
    MP_DECEL_DEC, // 정지전 가속도 완화
    MP_DONE //목적지 도착
} MP_State_t;

typedef struct {
    float target_pos;   //최종 목표 위치(pulse)
    float current_pos; //실시간 계산된 목표 위치 (PID 입력값)
    float velocity; //현재 속도
    float acceleration; //현재 가속도

    float max_vel;  //설정된 최대 속도
    float max_accel; //설정된 최대 가속도
    float jerk; //가속도 변화율(jerk)

    float dt;   //연산 주기 (10ms)

    float state_timer; // 현재 상태가 시작된 후 흐른 시간을 저장.

    MP_State_t state;
} MotionPlanner_t;

void MP_Init(MotionPlanner_t *p, float max_v, float max_a, float j, float dt);
void MP_SetTarget(MotionPlanner_t *p, float target);
float MP_Update(MotionPlanner_t *p);

#endif
