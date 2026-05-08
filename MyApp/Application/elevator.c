#include "elevator.h"

#include <string.h>

#include "elevator_controller.h"

static void elevatorSetState(elevator_t *ctx, elevator_state_t state, uint32_t now)
{
    if (ctx == NULL) {
        return;
    }

    ctx->state = state;
    ctx->state_time = now;
    ctx->entry_executed = false;
}

static bool elevatorHasRequestAtFloor(const bsp_elevator_input_t *input, uint8_t floor)
{
    if (input == NULL || floor < 1U || floor > ELEVATOR_FLOOR_MAX) {
        return false;
    }

    return input->call_car[floor] ||
           input->call_up[floor] ||
           input->call_down[floor] ||
           input->req_wifi[floor - 1U];
}

uint8_t elevatorFindNextTarget(const elevator_t *ctx, const bsp_elevator_input_t *input, bsp_lift_dir_t current_dir)
{
    uint8_t curr_floor;

    if (ctx == NULL || input == NULL) {
        return 0;
    }

    curr_floor = ctx->curr_floor;

    if (curr_floor < 1U || curr_floor > ELEVATOR_FLOOR_MAX) {
        curr_floor = 1U;
    }

    if (current_dir == BSP_LIFT_UP || current_dir == BSP_LIFT_STOP) {
        for (uint8_t floor = curr_floor + 1U; floor <= ELEVATOR_FLOOR_MAX; floor++) {
            if (input->call_car[floor] || input->req_wifi[floor - 1U]) {
                return floor;
            }
        }

        for (uint8_t floor = curr_floor + 1U; floor <= ELEVATOR_FLOOR_MAX; floor++) {
            if (input->call_up[floor] || input->call_down[floor]) {
                return floor;
            }
        }
    }

    if (current_dir == BSP_LIFT_DOWN || current_dir == BSP_LIFT_STOP) {
        for (int floor = (int)curr_floor - 1; floor >= 1; floor--) {
            if (input->call_car[floor] || input->req_wifi[floor - 1]) {
                return (uint8_t)floor;
            }
        }

        for (int floor = (int)curr_floor - 1; floor >= 1; floor--) {
            if (input->call_up[floor] || input->call_down[floor]) {
                return (uint8_t)floor;
            }
        }
    }

    if (current_dir == BSP_LIFT_UP) {
        for (int floor = (int)curr_floor - 1; floor >= 1; floor--) {
            if (elevatorHasRequestAtFloor(input, (uint8_t)floor)) {
                return (uint8_t)floor;
            }
        }
    }
    else if (current_dir == BSP_LIFT_DOWN) {
        for (uint8_t floor = curr_floor + 1U; floor <= ELEVATOR_FLOOR_MAX; floor++) {
            if (elevatorHasRequestAtFloor(input, floor)) {
                return floor;
            }
        }
    }
    else {
        uint8_t up_floor = 0;
        uint8_t down_floor = 0;

        for (uint8_t floor = curr_floor + 1U; floor <= ELEVATOR_FLOOR_MAX; floor++) {
            if (elevatorHasRequestAtFloor(input, floor)) {
                up_floor = floor;
                break;
            }
        }

        for (int floor = (int)curr_floor - 1; floor >= 1; floor--) {
            if (elevatorHasRequestAtFloor(input, (uint8_t)floor)) {
                down_floor = (uint8_t)floor;
                break;
            }
        }

        if (up_floor != 0U && down_floor != 0U) {
            return ((up_floor - curr_floor) <= (curr_floor - down_floor)) ? up_floor : down_floor;
        }

        if (up_floor != 0U) {
            return up_floor;
        }

        if (down_floor != 0U) {
            return down_floor;
        }
    }

    return 0;
}

static bsp_lift_dir_t elevatorGetServiceDir(const elevator_t *ctx, const bsp_elevator_input_t *input)
{
    if (ctx == NULL || input == NULL) {
        return BSP_LIFT_STOP;
    }

    if (ctx->curr_floor == ELEVATOR_FLOOR_MAX) {
        return BSP_LIFT_DOWN;
    }

    if (ctx->curr_floor == 1U) {
        return BSP_LIFT_UP;
    }

    if (input->current_dir == BSP_LIFT_UP && input->call_up[ctx->curr_floor]) {
        return BSP_LIFT_UP;
    }

    if (input->current_dir == BSP_LIFT_DOWN && input->call_down[ctx->curr_floor]) {
        return BSP_LIFT_DOWN;
    }

    if (input->call_up[ctx->curr_floor]) {
        return BSP_LIFT_UP;
    }

    if (input->call_down[ctx->curr_floor]) {
        return BSP_LIFT_DOWN;
    }

    return BSP_LIFT_STOP;
}

void elevatorUpdate(elevator_t *ctx, uint32_t now)
{
    bsp_elevator_input_t input;

    if (ctx == NULL) {
        return;
    }

    bspElevatorReadInput(&input);

    if (input.floor_valid && input.curr_floor != 0U) {
        ctx->curr_floor = input.curr_floor;
    }

    if (input.emergency_stop || input.motor_over_current || input.top_limit || input.bottom_limit) {
        bspLiftMotorSet(BSP_LIFT_STOP, 0);
        bspSetCurrentDir(BSP_LIFT_STOP);
        bspSetSpecialState(BSP_STATE_NORMAL);
        elevatorSetState(ctx, CAR_STATE_ERROR, now);
    }

    switch (ctx->state) {
    case CAR_STATE_IDLE:
        bspLiftMotorSet(BSP_LIFT_STOP, 0);
        bspSetCurrentDir(BSP_LIFT_STOP);
        bspSetSpecialState(BSP_STATE_NORMAL);

        ctx->target_floor = elevatorFindNextTarget(ctx, &input, BSP_LIFT_STOP);
        if (ctx->target_floor == 0U) {
            break;
        }

        if (ctx->target_floor == ctx->curr_floor) {
            elevatorSetState(ctx, CAR_STATE_DOOR_OPENING, now);
        }
        else if (ctx->target_floor > ctx->curr_floor) {
            bspSetCurrentDir(BSP_LIFT_UP);
            ctx->start_floor = ctx->curr_floor;
            if (Elevator_GoToFloor(ctx->target_floor)) {
                elevatorSetState(ctx, CAR_STATE_MOV_UP, now);
            }
            else {
                elevatorSetState(ctx, CAR_STATE_ERROR, now);
            }
        }
        else {
            bspSetCurrentDir(BSP_LIFT_DOWN);
            ctx->start_floor = ctx->curr_floor;
            if (Elevator_GoToFloor(ctx->target_floor)) {
                elevatorSetState(ctx, CAR_STATE_MOV_DOWN, now);
            }
            else {
                elevatorSetState(ctx, CAR_STATE_ERROR, now);
            }
        }
        break;

    case CAR_STATE_MOV_UP:
    case CAR_STATE_MOV_DOWN:
        bspSetSpecialState(BSP_STATE_MOVING);

        if (Elevator_IsBusy() == false) {
            ctx->curr_floor = ctx->target_floor;
            bspSetCurrentFloor(ctx->target_floor);
            bspSetSpecialState(BSP_STATE_NORMAL);
            bspSetArrivedAck(false);
            (void)bspCanSendArrivedCheck(ctx->curr_floor);
            elevatorSetState(ctx, CAR_STATE_WAIT_SENSOR_ACK, now);
        }
        break;

    case CAR_STATE_WAIT_SENSOR_ACK:
        bspElevatorReadInput(&input);
        if (input.arrived_ack) { // || (now - ctx->state_time > SENSOR_ACK_TIMEOUT_MS) // disable timeout due to safety
            elevatorSetState(ctx, CAR_STATE_DOOR_OPENING, now);
        }
        break;

    case CAR_STATE_DOOR_OPENING:
        if (ctx->entry_executed == false) {
            bsp_lift_dir_t service_dir;

            ctx->entry_executed = true;
            bspElevatorReadInput(&input);
            service_dir = elevatorGetServiceDir(ctx, &input);

            (void)bspCanSendDoorCmd(ctx->curr_floor, DOOR_CMD_OPEN, (uint8_t)service_dir);
            bspClearServicedRequests(ctx->curr_floor, service_dir);
        }

        elevatorSetState(ctx, CAR_STATE_DOOR_OPEN, now);
        break;

    case CAR_STATE_DOOR_OPEN:
        if (now - ctx->state_time > DOOR_OPEN_TIME_MS) {
            elevatorSetState(ctx, CAR_STATE_DOOR_CLOSING, now);
        }
        break;

    case CAR_STATE_DOOR_CLOSING:
        if (ctx->entry_executed == false) {
            ctx->entry_executed = true;
            (void)bspCanSendDoorCmd(ctx->curr_floor, DOOR_CMD_CLOSE, BSP_LIFT_STOP);
        }

        if (now - ctx->state_time > DOOR_MOVE_TIMEOUT_MS) {
            elevatorSetState(ctx, CAR_STATE_IDLE, now);
        }
        break;

    case CAR_STATE_ERROR:
    default:
        bspLiftMotorSet(BSP_LIFT_STOP, 0);
        bspSetCurrentDir(BSP_LIFT_STOP);
        bspSetSpecialState(BSP_STATE_NORMAL);
        break;
    }
}

void elevatorInit(elevator_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->state = CAR_STATE_IDLE;
    ctx->curr_floor = 1;
    ctx->target_floor = 0;
    ctx->start_floor = 0;
    ctx->req_mask = 0;
    ctx->state_time = 0;
    ctx->entry_executed = false;
}
