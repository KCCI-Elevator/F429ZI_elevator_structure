#include "elevator.h"
#include "bsp.h"

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
}

void elevatorUpdate(elevator_t *ctx, uint32_t now) {
    bsp_elevator_input_t input;

    bspElevatorReadInput(&input);

    if (input.floor_valid == true) {
        ctx->curr_floor = input.curr_floor;
    }

    ctx->req_mask |= input.req_mask;

    if (input.emergency_stop == true || input.motor_over_current == true) {
        bspLiftMotorSet(BSP_LIFT_STOP, 0);
        bspDoorMotorSet(BSP_DOOR_STOP, 0);
        elevatorSetState(ctx, CAR_STATE_ERROR, now);
    }

    switch (ctx->state) {
        case CAR_STATE_IDLE:
            bspLiftMotorSet(BSP_LIFT_STOP, 0);
            bspDoorMotorSet(BSP_DOOR_STOP, 0);

            ctx->target_floor = elevatorFindNextTarget(ctx);

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
    }
}