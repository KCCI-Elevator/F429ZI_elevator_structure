#include "elevator.h"
#include "motion_unit_conv.h"
#include "bsp.h"
#include "motor.h"
#include "elevator_controller.h"
#include "elevator_param.h"
#include <stdio.h>
#include <math.h>


// inner
static void elevatorSetState(elevator_t *ctx, elevator_state_t state, uint32_t now) {
    ctx->state = state;
    ctx->state_time = now;
}

static uint8_t elevatorFindNextTarget(elevator_t *ctx) {
    for (uint8_t floor = 1; floor <= ELEVATOR_FLOOR_MAX; floor++) {
        if (ctx->req_mask & FLOOR_BIT(floor)) return floor;
    }
    return 0;
}

// function
void elevatorInit(elevator_t *ctx) {
    ctx->state = CAR_STATE_IDLE;
    ctx->curr_floor = 1;
    ctx->target_floor = 0;
    ctx->req_mask = 0;
    ctx->state_time = 0;

    bspLiftMotorSet(BSP_LIFT_STOP, 0);
    bspDoorMotorSet(BSP_DOOR_STOP, 0);
    Elevator_Controller_Init();
}

void elevatorUpdate(elevator_t *ctx, uint32_t now) {
    
    bsp_elevator_input_t input;

    
    bspElevatorReadInput(&input);

    /*
     * 현재 운전은 UART 요청 기반으로 동작.
     * 센서 입력(층/버튼)이 부동 상태이면 curr_floor/request를 덮어써
     * 출발 조건(target!=current)을 깨뜨리므로 여기서는 반영하지 않는다.
     * (층 정보는 도착 판정 시 소프트웨어적으로 갱신)
     */

    if (input.emergency_stop == true || input.motor_over_current == true) {
       
        bspLiftMotorSet(BSP_LIFT_STOP, 0);
        bspDoorMotorSet(BSP_DOOR_STOP, 0);
        elevatorSetState(ctx, CAR_STATE_ERROR, now);
    }
        

    switch (ctx->state) {
    
        case CAR_STATE_IDLE:

            ctx->target_floor = elevatorFindNextTarget(ctx);
            if(ctx->target_floor!=0 && ctx->target_floor!=ctx->curr_floor){
                //목표 거리 계산 및 Scurve 초기화
                float dist = fabsf(motionFloorToMM(ctx->target_floor) - motionFloorToMM(ctx->curr_floor));

                Elevator_Controller_SetTarget(dist);

                ctx->state = (ctx->target_floor > ctx->curr_floor) ? CAR_STATE_MOV_UP : CAR_STATE_MOV_DOWN;


            }
            break;

            case CAR_STATE_MOV_UP:
            case CAR_STATE_MOV_DOWN:
                uint8_t dir_flag = (ctx->state == CAR_STATE_MOV_UP) ? 1 : 0;
                bool arrived = Elevator_Controller_Update(dir_flag);
                if(arrived) {
                    ctx->curr_floor = ctx->target_floor;
                    ctx->req_mask &= ~FLOOR_BIT(ctx->curr_floor);
                    ctx->state = CAR_STATE_IDLE;
                }
                break;

              

            default:
                    // 도어 열림/닫힘 등은 현재 IDLE로 리턴시켜 기능 제한
                    if(ctx->state != CAR_STATE_ERROR) ctx->state = CAR_STATE_IDLE;
                    break;
/*

            if (ctx->target_floor == 0) {
                break;
            }

            if (ctx->target_floor == ctx->curr_floor) {
                ctx->req_mask &= ~FLOOR_BIT(ctx->curr_floor);
                elevatorSetState(ctx, CAR_STATE_DOOR_OPENING, now);
            }
            else if (ctx->target_floor > ctx->curr_floor) {
                bspLiftMotorSet(BSP_LIFT_UP, 700);
                elevatorSetState(ctx, CAR_STATE_MOV_UP, now);
            }
            else {
                bspLiftMotorSet(BSP_LIFT_DOWN, 700);
                elevatorSetState(ctx, CAR_STATE_MOV_DOWN, now);
            }
            break;

        case CAR_STATE_MOV_UP:
            if (input.top_limit == true) {
                bspLiftMotorSet(BSP_LIFT_STOP, 0);
                elevatorSetState(ctx, CAR_STATE_ERROR, now);
                break;
            }

            if (ctx->curr_floor == ctx->target_floor) {
                bspLiftMotorSet(BSP_LIFT_STOP, 0);
                ctx->req_mask &= ~FLOOR_BIT(ctx->curr_floor);
                elevatorSetState(ctx, CAR_STATE_DOOR_OPENING, now);
            }
            break;

        case CAR_STATE_MOV_DOWN:
            if (input.bottom_limit == true) {
                bspLiftMotorSet(BSP_LIFT_STOP, 0);
                elevatorSetState(ctx, CAR_STATE_ERROR, now);
                break;
            }

            if (ctx->curr_floor == ctx->target_floor) {
                bspLiftMotorSet(BSP_LIFT_STOP, 0);
                ctx->req_mask &= ~FLOOR_BIT(ctx->curr_floor);
                elevatorSetState(ctx, CAR_STATE_DOOR_OPENING, now);
            }
            break;

        case CAR_STATE_DOOR_OPENING:
            bspDoorMotorSet(BSP_DOOR_OPEN, 500);

            if (input.door_open_limit == true) {
                bspDoorMotorSet(BSP_DOOR_STOP, 0);
                elevatorSetState(ctx, CAR_STATE_DOOR_OPEN, now);
            }
            else if (now - ctx->state_time > DOOR_MOVE_TIMEOUT_MS) {
                bspDoorMotorSet(BSP_DOOR_STOP, 0);
                elevatorSetState(ctx, CAR_STATE_ERROR, now);
            }
            break;

        case CAR_STATE_DOOR_OPEN:
            bspDoorMotorSet(BSP_DOOR_STOP, 0);

            if (now - ctx->state_time > DOOR_OPEN_TIME_MS) {
                elevatorSetState(ctx, CAR_STATE_DOOR_CLOSING, now);
            }
            break;

        case CAR_STATE_DOOR_CLOSING:
            if (input.obstacle_detected == true) {
                elevatorSetState(ctx, CAR_STATE_DOOR_OPENING, now);
                break;
            }

            bspDoorMotorSet(BSP_DOOR_CLOSE, 500);

            if (input.door_close_limit == true) {
                bspDoorMotorSet(BSP_DOOR_STOP, 0);
                elevatorSetState(ctx, CAR_STATE_IDLE, now);
            }
            else if (now - ctx->state_time > DOOR_MOVE_TIMEOUT_MS) {
                bspDoorMotorSet(BSP_DOOR_STOP, 0);
                elevatorSetState(ctx, CAR_STATE_ERROR, now);
            }
            break;

        case CAR_STATE_ERROR:
        default:
            bspLiftMotorSet(BSP_LIFT_STOP, 0);
            bspDoorMotorSet(BSP_DOOR_STOP, 0);
            break;
            */
    }
}