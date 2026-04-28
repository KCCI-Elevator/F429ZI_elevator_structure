#include "ap.h"

// inner
static elevator_t elevator;

// task Init
void StartDefaultTask(void *argument) {
    apInit();

    while (1) {
        apMain();
    }
}

void motorTask(void *argument) {
    while (1) {
        osDelay(1);
    }
}

// function
void apInit(void) {
    bspInit();

    elevatorInit(&elevator);
}

void apMain(void) {
    uint32_t prev_time = bspMillis();

    while (1) {
        // action
        uint32_t now = bspMillis();

        if (now - prev_time >= 10){
            prev_time = now;

            bspUpdate();
            elevatorUpdate(&elevator, now);
        }
        bspDelay(1);
    }
}