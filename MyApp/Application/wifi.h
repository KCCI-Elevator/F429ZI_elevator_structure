#ifndef __MAP_AP__WIFI_H__
#define __MAP_AP__WIFI_H__

#include "def.h"
#include "esp8266.h"

typedef enum {
    WIFI_STATE_INIT = 0,
    WIFI_STATE_AT_CHECK,
    WIFI_STATE_SET_MODE,
    WIFI_STATE_JOIN_AP,
    WIFI_STATE_CONNECT_SERVER,
    WIFI_STATE_READY,
    WIFI_STATE_ERROR,
    WIFI_STATE_RETRY
} wifi_state_t;

typedef struct {
    esp8266_t *esp;

    wifi_state_t state;
    uint32_t prev_time;
    uint8_t connected;
} wifi_t;

void wifiInit(wifi_t *ctx, esp8266_t *esp);
void wifiProcess(wifi_t *ctx);

uint8_t wifiIsConnected(wifi_t *ctx);

void wifiSendElevatorStatus(
    wifi_t *ctx,
    uint8_t floor,
    uint8_t target_floor,
    uint8_t door_open,
    uint8_t motor_state
);

#endif //__MAP_AP__WIFI_H__