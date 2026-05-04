#include "wifi.h"

#include <stdio.h>

#define WIFI_SSID       "KCCI_STC_S"
#define WIFI_PASSWORD   "kcci098#"
#define SERVER_IP       "192.168.0.10"
#define SERVER_PORT     5000

// function
void wifiInit(wifi_t *ctx, esp8266_t *esp) {
    ctx->esp = esp;
    ctx->state = WIFI_STATE_INIT;
    ctx->prev_time = 0;
    ctx->connected = 0;
}

void wifiProcess(wifi_t *ctx) {
    esp8266_Process(ctx->esp);

    switch (ctx->state) {
        case WIFI_STATE_INIT:
            ctx->state = WIFI_STATE_AT_CHECK;
            break;

        case WIFI_STATE_AT_CHECK:
            if (esp8266_TestAT(ctx->esp) == ESP8266_OK) {
                ctx->state = WIFI_STATE_SET_MODE;
            }
            else {
                ctx->state = WIFI_STATE_ERROR;
            }
            break;

        case WIFI_STATE_SET_MODE:
            if (esp8266_SetMode(ctx->esp, ESP8266_WIFI_MODE_STA) == ESP8266_OK) {
                ctx->state = WIFI_STATE_JOIN_AP;
            }
            else {
                ctx->state = WIFI_STATE_ERROR;
            }
            break;

        case WIFI_STATE_JOIN_AP:
            if (esp8266_JoinAP(ctx->esp, WIFI_SSID, WIFI_PASSWORD) == ESP8266_OK) {
                ctx->state = WIFI_STATE_CONNECT_SERVER;
            }
            else {
                ctx->state = WIFI_STATE_ERROR;
            }
            break;

        case WIFI_STATE_CONNECT_SERVER:
            if (esp8266_StartTCP(ctx->esp, SERVER_IP, SERVER_PORT) == ESP8266_OK) {
                ctx->connected = 1;
                ctx->state = WIFI_STATE_READY;
            }
            else {
                ctx->state = WIFI_STATE_ERROR;
            }
            break;

        case WIFI_STATE_READY:
            break;

        case WIFI_STATE_ERROR:
            ctx->connected = 0;
            //ctx->prev_time = HAL_GetTick();
            ctx->state = WIFI_STATE_RETRY;
            break;

        case WIFI_STATE_RETRY:
            /*
             * 일정 시간 후 재시도하도록 만들면 됨.
             * 처음에는 단순하게 INIT으로 돌려도 됨.
             */
            ctx->state = WIFI_STATE_INIT;
            break;

        default:
            ctx->state = WIFI_STATE_INIT;
            break;
    }
}

uint8_t wifiIsConnected(wifi_t *ctx) {
    if (ctx == NULL) return 0;
    
    return ctx->connected;
}

void wifiSendElevatorStatus(
    wifi_t *ctx,
    uint8_t floor,
    uint8_t target_floor,
    uint8_t door_open,
    uint8_t motor_state
){
    char tx_buf[128];

    if (ctx == NULL || ctx->esp == NULL) return;
    if (ctx->connected == 0) return;

    snprintf(tx_buf, sizeof(tx_buf),
             "floor=%u,target=%u,door=%u,motor=%u\r\n",
             (unsigned int)floor,
             (unsigned int)target_floor,
             (unsigned int)door_open,
             (unsigned int)motor_state);

    esp8266_SendData(ctx->esp, tx_buf);
}