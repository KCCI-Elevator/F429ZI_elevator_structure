//
// Created by hiimseoll on 26. 5. 6..
//

#include "wifi.h"
#include "esp8266.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmsis_os2.h"
#include "main.h"

// Wi-Fi 접속 정보
#define WIFI_SSID       "KCCI_STC_S"
#define WIFI_PASSWORD   "kcci098#"

// Blynk 서버 설정
#define BLYNK_SERVER    "blynk.cloud"
#define BLYNK_PORT      80
#define BLYNK_TOKEN     "ryky9TPZIIm5DPqhqq7_9FgcXiTMa2ou"

wifi_t wifi_ctx;
esp8266_t esp8266_ctx;

// JSON 응답에서 특정 핀(v1, v2 등)의 값을 찾아 정수로 반환하는 헬퍼 함수
static int parse_blynk_val(const char* body, const char* key) {
    char search_key[16];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    char *p = strstr(body, search_key);

    if (p) {
        p += strlen(search_key);
        while (*p && (*p < '0' || *p > '9') && *p != '-') p++;
        if (*p) return atoi(p);
    }
    return -1;
}

void wifiInit(wifi_t *ctx, esp8266_t *esp) {
    ctx->esp = esp;
    ctx->state = WIFI_STATE_INIT;
    ctx->prev_time = 0;
    ctx->connected = 0;
}

void wifiProcess(wifi_t *ctx) {
    esp8266_Process(ctx->esp);

    // 서버 끊김 감지 및 재연결
    if (ctx->state == WIFI_STATE_READY) {
        if (strstr(ctx->esp->rx_buf, "CLOSED") != NULL) {
            ctx->connected = 0;
            ctx->state = WIFI_STATE_CONNECT_SERVER;
            memset(ctx->esp->rx_buf, 0, sizeof(ctx->esp->rx_buf));
            ctx->esp->rx_len = 0;
        }
    }

    switch (ctx->state) {
        case WIFI_STATE_INIT:
            ctx->state = WIFI_STATE_AT_CHECK;
            break;
        case WIFI_STATE_AT_CHECK:
            if (esp8266_TestAT(ctx->esp) == ESP8266_OK) ctx->state = WIFI_STATE_SET_MODE;
            else ctx->state = WIFI_STATE_ERROR;
            break;
        case WIFI_STATE_SET_MODE:
            if (esp8266_SetMode(ctx->esp, ESP8266_WIFI_MODE_STA) == ESP8266_OK) ctx->state = WIFI_STATE_JOIN_AP;
            else ctx->state = WIFI_STATE_ERROR;
            break;
        case WIFI_STATE_JOIN_AP:
            if (esp8266_JoinAP(ctx->esp, WIFI_SSID, WIFI_PASSWORD) == ESP8266_OK) ctx->state = WIFI_STATE_CONNECT_SERVER;
            else ctx->state = WIFI_STATE_ERROR;
            break;
        case WIFI_STATE_CONNECT_SERVER:
            if (esp8266_StartTCP(ctx->esp, BLYNK_SERVER, BLYNK_PORT) == ESP8266_OK) {
                ctx->connected = 1;
                ctx->state = WIFI_STATE_READY;
            } else {
                ctx->state = WIFI_STATE_ERROR;
            }
            break;
        case WIFI_STATE_READY:
            break;
        case WIFI_STATE_ERROR:
            ctx->connected = 0;
            ctx->state = WIFI_STATE_RETRY;
            break;
        case WIFI_STATE_RETRY:
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

// 엘리베이터 현재 층수를 V4 핀으로 송신
void wifiSendElevatorStatus(wifi_t *ctx, bsp_elevator_input_t *input) {
  char tx_buf[256];

  if (ctx == NULL || ctx->esp == NULL || input == NULL) return;
  if (ctx->connected == 0) return;

  // v4=%u 로 되어 있던 부분을 v0=%u 로 변경합니다.
  snprintf(tx_buf, sizeof(tx_buf),
           "GET /external/api/update?token=%s&v0=%u HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Connection: keep-alive\r\n\r\n",
           BLYNK_TOKEN, (unsigned int)input->curr_floor, BLYNK_SERVER);

  esp8266_SendData(ctx->esp, tx_buf);
}

// V1, V2, V3 핀 값을 한 번에 수신
int wifiReceiveCommands(wifi_t *ctx, int *v1, int *v2, int *v3) {
  char tx_buf[150];
  *v1 = -1; *v2 = -1; *v3 = -1;

  if (ctx == NULL || ctx->esp == NULL || ctx->connected == 0) return -1;

  // 수신 버퍼 깨끗하게 청소
  memset(ctx->esp->rx_buf, 0, sizeof(ctx->esp->rx_buf));
  ctx->esp->rx_len = 0;

  snprintf(tx_buf, sizeof(tx_buf),
           "GET /external/api/get?token=%s&v1&v2&v3 HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Connection: keep-alive\r\n\r\n",
           BLYNK_TOKEN, BLYNK_SERVER);

  if (esp8266_SendData(ctx->esp, tx_buf) == ESP8266_OK) {
    uint32_t start_time = HAL_GetTick();

    // 서버 응답 대기 (최대 1.2초)
    while ((HAL_GetTick() - start_time) < 1200) {
      esp8266_Process(ctx->esp);

      // 헤더 끝 지점 탐색
      char *body = strstr(ctx->esp->rx_buf, "\r\n\r\n");
      if (body != NULL) {
        body += 4; // 실제 데이터(본문) 시작

        // [강력한 검색 로직] JSON 구조 무시하고 키워드 위주로 검색
        char *p;
        // V1 검색
        if ((p = strstr(body, "\"v1\"")) != NULL) {
          p = strstr(p, ":"); // 콜론 찾기
          while (*p && (*p < '0' || *p > '9')) p++; // 숫자까지 전진
          if (*p) *v1 = *p - '0';
        }
        // V2 검색
        if ((p = strstr(body, "\"v2\"")) != NULL) {
          p = strstr(p, ":");
          while (*p && (*p < '0' || *p > '9')) p++;
          if (*p) *v2 = *p - '0';
        }
        // V3 검색
        if ((p = strstr(body, "\"v3\"")) != NULL) {
          p = strstr(p, ":");
          while (*p && (*p < '0' || *p > '9')) p++;
          if (*p) *v3 = *p - '0';
        }

        // 하나라도 0 또는 1을 찾았다면 성공 반환
        if (*v1 != -1 || *v2 != -1 || *v3 != -1) return 0;
      }
      osDelay(20);
    }
  }
  return -1;
}

// 지정된 핀(pin)의 값을 0으로 초기화
void wifiClearCommand(wifi_t *ctx, int pin) {
  char tx_buf[256];

  if (ctx == NULL || ctx->esp == NULL) return;
  if (ctx->connected == 0) return;

  snprintf(tx_buf, sizeof(tx_buf),
           "GET /external/api/update?token=%s&v%d=0 HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Connection: keep-alive\r\n\r\n",
           BLYNK_TOKEN, pin, BLYNK_SERVER);

  esp8266_SendData(ctx->esp, tx_buf);
  osDelay(50); // 패킷 꼬임 방지 딜레이
}