#include "ap.h"
#include "bsp.h"
#include "elevator.h"
#include "wifi.h"

#include <stdint.h>

#include "cmsis_os2.h"

// inner
static volatile bool app_ready = false;

static elevator_t elevator;
static wifi_t wifi;

// task Init
void StartDefaultTask(void *argument) {
    apInit();
    app_ready = true;

    apMain();
}

void motorTask(void *argument) {
    while (1) {
        osDelay(1);
    }
}

void wifiTask(void *argument) {
    while (app_ready == false){
        osDelay(1);
    }
    
    while (1) {
        wifiProcess(&wifi);
        osDelay(1);
    }
}

// function
void apInit(void) {
    if (bspInit() == false) {
        while (1) {
            osDelay(1000);
        }
    }

    wifiInit(&wifi, bspGetEsp8266());
    elevatorInit(&elevator);
}   // 에러 led 또는 로그 출력으로 바꿔도 됨

void apMain(void) {
    uint32_t prev_time = bspMillis();
    uint32_t prev_wifi_tx_time = bspMillis();

    while (1) {
        // action
        uint32_t now = bspMillis();

        // elevator action
        if (now - prev_time >= 10){
            prev_time = now;

            bspUpdate();
            elevatorUpdate(&elevator, now);
        }

        // wifi action 500ms마다
        if (now - prev_wifi_tx_time >= 500) {
            prev_wifi_tx_time = now;

            if (wifiIsConnected(&wifi)) {
                wifiSendElevatorStatus(&wifi,
                                       elevator.current_floor,
                                       elevator.target_floor,
                                       0,
                                       elevator.state);
            }
        } // blocking 구조라 wifiTask()로 이동시켜 처리하는 구조로 바꿔야함

        bspDelay(1);
    }
}