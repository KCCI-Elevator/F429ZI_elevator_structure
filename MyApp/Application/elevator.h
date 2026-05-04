#ifndef __MAP_AP__ELEVATOR_H__
#define __MAP_AP__ELEVATOR_H__

#include "def.h"
#include "bsp.h"

// define
#define ELEVATOR_FLOOR_MAX      3

#define FLOOR_BIT(floor)        (1U << ((floor) - 1U))

#define DOOR_OPEN_TIME_MS       2000
#define DOOR_MOVE_TIMEOUT_MS    3000

// typedef
typedef enum {
    CAR_STATE_IDLE = 0,
    CAR_STATE_MOV_UP,
    CAR_STATE_MOV_DOWN,
    CAR_STATE_DOOR_OPENING,
    CAR_STATE_DOOR_OPEN,
    CAR_STATE_DOOR_CLOSING,
    CAR_STATE_ERROR
} elevator_state_t;

typedef struct {
    elevator_state_t state;

    uint8_t current_floor;
    uint8_t target_floor;
    uint8_t request_mask;

    uint32_t state_time;
} elevator_t;

// function
void elevatorInit(elevator_t *ctx);
void elevatorUpdate(elevator_t *ctx, uint32_t now);

#endif //__MAP_AP__ELEVATOR_H__