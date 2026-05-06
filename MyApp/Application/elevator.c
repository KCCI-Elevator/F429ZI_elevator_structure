#include "elevator.h"
#include "bsp.h"

extern bsp_elevator_input_t elevator_input;

// my_can.c의 함수들 외부 선언
extern void send_car_status(void);
extern void send_can_arrived_check(uint8_t floor);
extern void send_can_door_cmd(uint8_t floor, uint8_t cmd, uint8_t dir);

/**
 * @brief 엘리베이터 상태 변경
 */
static void elevatorSetState(elevator_t *ctx, elevator_state_t state, uint32_t now) {
    ctx->state = state;
    ctx->state_time = now;
    ctx->entry_executed = false;
}

/**
 * @brief 다음 목표 층 찾기 (IDLE 상태에서 출발 목적지 검색)
 */
uint8_t elevatorFindNextTarget(elevator_t *ctx, bsp_elevator_input_t *input, uint8_t current_dir) {
    uint8_t curr_floor = ctx->curr_floor;

    // --- 1단계: 같은 방향의 호출 찾기 ---
    if (current_dir == BSP_LIFT_UP || current_dir == BSP_LIFT_STOP) {
        for (uint8_t floor = curr_floor + 1; floor <= 3; floor++) {
            // [수정] req_wifi[floor-1] 로 인덱스 보정 필수
            if (input->call_car[floor] || input->req_wifi[floor - 1]) return floor;
        }
        for (uint8_t floor = curr_floor + 1; floor <= 3; floor++) {
            if (input->call_up[floor] || input->call_down[floor]) return floor;
        }
    }

    if (current_dir == BSP_LIFT_DOWN || current_dir == BSP_LIFT_STOP) {
        for (int floor = (int)curr_floor - 1; floor >= 1; floor--) {
            // [수정] req_wifi[floor-1]
            if (input->call_car[floor] || input->req_wifi[floor - 1]) return floor;
        }
        for (int floor = (int)curr_floor - 1; floor >= 1; floor--) {
            if (input->call_up[floor] || input->call_down[floor]) return floor;
        }
    }

    // --- 2단계: 반대 방향의 호출 찾기 ---
    if (current_dir == BSP_LIFT_UP) {
        for (int floor = (int)curr_floor - 1; floor >= 1; floor--) {
            if (input->call_car[floor] || input->call_up[floor] ||
                input->call_down[floor] || input->req_wifi[floor - 1]) return floor;
        }
    }
    else if (current_dir == BSP_LIFT_DOWN) {
        for (uint8_t floor = curr_floor + 1; floor <= 3; floor++) {
            if (input->call_car[floor] || input->call_up[floor] ||
                input->call_down[floor] || input->req_wifi[floor - 1]) return floor;
        }
    }
    else {
        // --- 3단계: IDLE에서 가장 가까운 호출 찾기 ---
        uint8_t up_floor = 0, down_floor = 0;

        for (uint8_t floor = curr_floor + 1; floor <= 3; floor++) {
            if (input->call_car[floor] || input->call_up[floor] ||
                input->call_down[floor] || input->req_wifi[floor - 1]) {
                up_floor = floor; break;
            }
        }
        for (int floor = (int)curr_floor - 1; floor >= 1; floor--) {
            if (input->call_car[floor] || input->call_up[floor] ||
                input->call_down[floor] || input->req_wifi[floor - 1]) {
                down_floor = floor; break;
            }
        }

        if (up_floor != 0 && down_floor != 0) {
            return ((up_floor - curr_floor) <= (curr_floor - down_floor)) ? up_floor : down_floor;
        }
        if (up_floor != 0) return up_floor;
        if (down_floor != 0) return down_floor;
    }

    return 0;
}

/**
 * @brief [핵심] 이동 중 실시간 정지 조건 판단 (SCAN 알고리즘)
 */
static bool elevatorCheckStopCondition(uint8_t floor, bsp_lift_dir_t dir, bsp_elevator_input_t *input) {
    // 1. 내부 탑승객이 키패드로 누른 목적지인가? (무조건 멈춤)
    if (input->call_car[floor]) return true;

    // 2. 외부에서 탑승하려는 방향이 현재 진행 방향과 일치하는가?
    if (dir == BSP_LIFT_UP && input->call_up[floor]) return true;
    if (dir == BSP_LIFT_DOWN && input->call_down[floor]) return true;

    // 3. 진행 방향 앞쪽에 더 이상 처리할 호출이 남아있는가? (스캔 종점 판단)
    bool calls_ahead = false;
    if (dir == BSP_LIFT_UP) {
        for (uint8_t i = floor + 1; i <= 3; i++) {
            if (input->call_car[i] || input->call_up[i] || input->call_down[i]) {
                calls_ahead = true;
                break;
            }
        }
    } else if (dir == BSP_LIFT_DOWN) {
        for (int i = floor - 1; i >= 1; i--) {
            if (input->call_car[i] || input->call_up[i] || input->call_down[i]) {
                calls_ahead = true;
                break;
            }
        }
    }

    if (!calls_ahead) {
        return true;
    }

    return false; // 통과
}

/**
 * @brief 엘리베이터 상태 머신 업데이트
 */
void elevatorUpdate(elevator_t *ctx, uint32_t now) {
    bsp_elevator_input_t input;
    bspElevatorReadInput(&input);

    if (input.floor_valid && input.curr_floor != 0) {
        ctx->curr_floor = input.curr_floor;
    }

    switch (ctx->state) {
        case CAR_STATE_IDLE: {
            bspLiftMotorSet(BSP_LIFT_STOP, 0);
            elevator_input.current_dir = BSP_LIFT_STOP;
            elevator_input.special_state = BSP_STATE_NORMAL;

            ctx->target_floor = elevatorFindNextTarget(ctx, &input, BSP_LIFT_STOP);
            if (ctx->target_floor == 0) break;

            if (ctx->target_floor == ctx->curr_floor) {
                elevatorSetState(ctx, CAR_STATE_DOOR_OPENING, now);
            }
            else if (ctx->target_floor > ctx->curr_floor) {
                elevator_input.current_dir = BSP_LIFT_UP;
                bspLiftMotorSet(BSP_LIFT_UP, 700);
                ctx->start_floor = ctx->curr_floor;
                elevatorSetState(ctx, CAR_STATE_MOV_UP, now);
            }
            else {
                elevator_input.current_dir = BSP_LIFT_DOWN;
                bspLiftMotorSet(BSP_LIFT_DOWN, 700);
                ctx->start_floor = ctx->curr_floor;
                elevatorSetState(ctx, CAR_STATE_MOV_DOWN, now);
            }
            break;
        }

        case CAR_STATE_MOV_UP: {
            elevator_input.special_state = BSP_STATE_MOVING;

            if (ctx->curr_floor != ctx->start_floor) {
                if (elevatorCheckStopCondition(ctx->curr_floor, BSP_LIFT_UP, &input)) {
                    bspLiftMotorSet(BSP_LIFT_STOP, 0);
                    elevator_input.special_state = BSP_STATE_NORMAL;
                    elevator_input.arrived_ack = false;
                    send_can_arrived_check(ctx->curr_floor);
                    elevatorSetState(ctx, CAR_STATE_WAIT_SENSOR_ACK, now);
                }
            }
            break;
        }

        case CAR_STATE_MOV_DOWN: {
            elevator_input.special_state = BSP_STATE_MOVING;

            if (ctx->curr_floor != ctx->start_floor) {
                if (elevatorCheckStopCondition(ctx->curr_floor, BSP_LIFT_DOWN, &input)) {
                    bspLiftMotorSet(BSP_LIFT_STOP, 0);
                    elevator_input.special_state = BSP_STATE_NORMAL;
                    elevator_input.arrived_ack = false;
                    send_can_arrived_check(ctx->curr_floor);
                    elevatorSetState(ctx, CAR_STATE_WAIT_SENSOR_ACK, now);
                }
            }
            break;
        }

        case CAR_STATE_WAIT_SENSOR_ACK: {
            if (elevator_input.arrived_ack || (now - ctx->state_time > 2000)) {
                elevatorSetState(ctx, CAR_STATE_DOOR_OPENING, now);
            }
            break;
        }

        case CAR_STATE_DOOR_OPENING: {
            if (!ctx->entry_executed) {
                ctx->entry_executed = true;

                uint8_t service_dir = BSP_LIFT_STOP;

                if (ctx->curr_floor == 3) service_dir = BSP_LIFT_DOWN;
                else if (ctx->curr_floor == 1) service_dir = BSP_LIFT_UP;
                else {
                    if (elevator_input.current_dir == BSP_LIFT_UP && input.call_up[ctx->curr_floor]) service_dir = BSP_LIFT_UP;
                    else if (elevator_input.current_dir == BSP_LIFT_DOWN && input.call_down[ctx->curr_floor]) service_dir = BSP_LIFT_DOWN;
                    else if (input.call_up[ctx->curr_floor]) service_dir = BSP_LIFT_UP;
                    else if (input.call_down[ctx->curr_floor]) service_dir = BSP_LIFT_DOWN;
                }

                send_can_door_cmd(ctx->curr_floor, DOOR_CMD_OPEN, service_dir);

                // 외부 승강장 버튼 취소
                if (service_dir == BSP_LIFT_UP) elevator_input.call_up[ctx->curr_floor] = false;
                else if (service_dir == BSP_LIFT_DOWN) elevator_input.call_down[ctx->curr_floor] = false;

                // [수정] 해당 층에 도착했으므로, 카 내부의 행선지 버튼(call_car)도 소거
                elevator_input.call_car[ctx->curr_floor] = false;
            }

            elevatorSetState(ctx, CAR_STATE_DOOR_OPEN, now);
            break;
        }

        case CAR_STATE_DOOR_OPEN: {
            if (now - ctx->state_time > DOOR_OPEN_TIME_MS) {
                elevatorSetState(ctx, CAR_STATE_DOOR_CLOSING, now);
            }
            break;
        }

        case CAR_STATE_DOOR_CLOSING: {
            if (!ctx->entry_executed) {
                ctx->entry_executed = true;
                send_can_door_cmd(ctx->curr_floor, DOOR_CMD_CLOSE, BSP_LIFT_STOP);
            }

            if (now - ctx->state_time > DOOR_MOVE_TIMEOUT_MS) {
                elevatorSetState(ctx, CAR_STATE_IDLE, now);
            }
            break;
        }

        case CAR_STATE_ERROR:
        default: {
            bspLiftMotorSet(BSP_LIFT_STOP, 0);
            elevator_input.current_dir = BSP_LIFT_STOP;
            elevator_input.special_state = BSP_STATE_NORMAL;
            break;
        }
    }
}

/**
 * @brief 엘리베이터 초기화
 */
void elevatorInit(elevator_t *ctx) {
    if (ctx == NULL) return;

    memset(ctx, 0, sizeof(elevator_t));
    ctx->state = CAR_STATE_IDLE;
    ctx->curr_floor = 1;
    ctx->target_floor = 0;
    ctx->start_floor = 0;
    ctx->req_mask = 0;
    ctx->state_time = 0;
    ctx->entry_executed = false;
}