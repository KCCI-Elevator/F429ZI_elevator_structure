
#include "Scurve.h"
#include <math.h>
#include <string.h>

//최대 속도, 최대 가속도, jerk, 주기 초기화.
void MP_Init(MotionPlanner_t *p, float max_v, float max_a, float j, float dt) {
    memset(p,0,sizeof(MotionPlanner_t));
    p->max_vel = max_v;
    p->max_accel = max_a;
    p->jerk = j;
    p->dt= dt;
    p->state = MP_IDLE;
}

//목표 위치 설정
void MP_SetTarget(MotionPlanner_t *p, float target) {
    p->target_pos = target;
    p->state = MP_ACCEL_INC; 
}

float MP_Update(MotionPlanner_t *p) {
    if(p->state == MP_IDLE) return p->current_pos;

    // 간단한 구간 제어 로직
    //목표위치랑 현재 위치의 차이 계산
    float dist_to_go = p->target_pos - p->current_pos;

    switch (p->state) {
        case MP_IDLE:
            break;

        case MP_ACCEL_INC: //가속도 증가 구간.
            p->acceleration += p->jerk * p->dt;
            if(p->acceleration >= p->max_accel)p->state = MP_ACCEL_DEC;
            break;

       /*case MP_ACCEL_CONST:
            p->acceleration = p->max_accel; //가속도 유지
             //시간 누적.
             p->state_timer += p->dt;

             //0.5초 정도 유지

             if(p->state_timer >= 0.5f) {
                p->state_timer = 0;
                p->state = MP_ACCEL_DEC;
             }
             break;*/
        

        case MP_ACCEL_DEC:
            p->acceleration -= p->jerk * p->dt; //가속도 깎기.
            if(p->acceleration <= 0) {
                p->acceleration = 0;
                p->state = MP_DECEL_INC;
            }
            break;
        
        /*case MP_CRUISE:
            p->acceleration = 0;
            // 제동 거리 계산
            //여유를 두고 감속.v^2=v0^2+as 공식 사용.
            float t_jerk = p->max_accel / p->jerk; //가속도가 max_a에서 0이 될때까지 걸리는 시간
            //scurve 전용 제동 거리 근사식
            float braking_distance = (p->velocity*p->velocity/(2*p->max_accel))+(p->velocity * t_jerk);
            if(fabs(dist_to_go) < braking_distance*1.1f){
                p->state =MP_DECEL_INC;
            }
            break;
            */
        
        case MP_DECEL_INC:
            p->acceleration -= p->jerk * p->dt;
            if(p->acceleration <= -p->max_accel) p->state = MP_DECEL_DEC;
            break;

         /*case MP_DECEL_CONST:
            p->acceleration = -p->max_accel; //가속도 유지
             //시간 누적.
            if(p->velocity <= (p->max_accel * p->max_accel / (2*p->jerk))){
                p->state = MP_DECEL_DEC;
            }
             break;*/
        
        case MP_DECEL_DEC:
        p->acceleration += p->jerk * p->dt;
            if(p->acceleration >= 0 || p->velocity <= 2.0f){
                p->velocity = 0;
                p->acceleration = 0;
                p->state = MP_DONE;

            }
            break;
        
        case MP_DONE:
            p->state = MP_IDLE;
            break;
            
    }

    //적분 연산 핵심.
    p->velocity += p->acceleration * p->dt;
    p->current_pos += p->velocity * p->dt;

    return p->current_pos;
}