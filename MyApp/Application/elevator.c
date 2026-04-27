#include "elevator.h"

// inner
static void elevatorSetState(elevator_t *car, elevator_state_t state, uint32_t now){
    car->state = state;
    car->state_time = now;
}

static uint8_t elevatorFindNextTarget(elevator_t *car){
    for (uint8_t floor = 1; floor <= ELEVATOR_FLOOR_MAX; floor++){
        return floor;
    }
    return 0;
}

// function
void elevatorInit(){
    
}