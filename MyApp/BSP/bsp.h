#ifndef __MAP_BSP__BSP_H_
#define __MAP_BSP__BSP_H_

#include <string.h>

#include "def.h"
#include "hw.h"
#include "motor.h"

// typedef
typedef enum {
    BSP_LIFT_STOP = 0,
    BSP_LIFT_UP,
    BSP_LIFT_DOWN
} bsp_lift_dir_t;

typedef enum {
    BSP_DOOR_STOP = 0,
    BSP_DOOR_OPEN,
    BSP_DOOR_CLOSE
} bsp_door_dir_t;

typedef enum {
    BSP_STATE_NORMAL = 0,
    BSP_STATE_INSPECTION,
    BSP_STATE_MOVING
} bsp_special_state_t;

typedef struct {
    bool floor_valid;
    uint8_t current_floor;

    uint8_t request_mask;

    bool top_limit;
    bool bottom_limit;

    bool door_open_limit;
    bool door_close_limit;
    bool obstacle_detected;

    bool emergency_stop;
    bool motor_over_current;
    
    bsp_special_state_t special_state; 
} bsp_elevator_input_t;

// function
void bspInit(void);
void bspUpdate(void);

uint32_t bspMillis(void);
void bspDelay(uint32_t delay_ms);


#endif //__MAP_BSP__BSP_H_
