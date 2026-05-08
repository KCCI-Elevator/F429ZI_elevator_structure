#ifndef __MAP_AP__ELEVATOR_H__
#define __MAP_AP__ELEVATOR_H__

#include "def.h"
#include "bsp.h"

#define ELEVATOR_FLOOR_MAX      3U
#define FLOOR_BIT(floor)        (1U << ((floor) - 1U))

#define DOOR_OPEN_TIME_MS       3000U
#define DOOR_MOVE_TIMEOUT_MS    3000U
#define SENSOR_ACK_TIMEOUT_MS   2000U

typedef enum {
    CAR_STATE_IDLE = 0,
    CAR_STATE_MOV_UP,
    CAR_STATE_MOV_DOWN,
    CAR_STATE_WAIT_SENSOR_ACK,
    CAR_STATE_DOOR_OPENING,
    CAR_STATE_DOOR_OPEN,
    CAR_STATE_DOOR_CLOSING,
    CAR_STATE_ERROR
} elevator_state_t;

typedef struct {
    elevator_state_t state;
    uint8_t curr_floor;
    uint8_t target_floor;
    uint8_t start_floor;
    uint8_t req_mask;
    uint32_t state_time;
    bool entry_executed;
} elevator_t;

void elevatorInit(elevator_t *ctx);
void elevatorUpdate(elevator_t *ctx, uint32_t now);
uint8_t elevatorFindNextTarget(const elevator_t *ctx, const bsp_elevator_input_t *input, bsp_lift_dir_t current_dir);

#endif // __MAP_AP__ELEVATOR_H__
