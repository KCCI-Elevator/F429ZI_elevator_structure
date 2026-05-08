#include "wifi.h"

//extern bsp_elevator_input_t elevator_input;

#define WIFI_SSID "KCCI_STC_S"
#define WIFI_PASSWORD "kcci098#"

#define BLYNK_SERVER "blynk.cloud"
#define BLYNK_PORT 80
#define BLYNK_TOKEN "ryky9TPZIIm5DPqhqq7_9FgcXiTMa2ou"

#define WIFI_RETRY_INTERVAL_MS 3000U

static wifi_t s_wifi_ctx;
static esp8266_t s_esp8266_ctx;

wifi_t *wifiGetDefaultContext(void) {
    return &s_wifi_ctx;
}

esp8266_t *wifiGetDefaultEsp(void) {
    return &s_esp8266_ctx;
}

bool wifiDefaultInit(void) {
    if (uartInit() == false) {
        return false;
    }

    esp8266_Init(&s_esp8266_ctx, UART_CH_ESP8266);
    wifiInit(&s_wifi_ctx, &s_esp8266_ctx);

    return true;
}

void wifiInit(wifi_t *ctx, esp8266_t *esp) {
    if (ctx == NULL) {
        return;
    }

    ctx->esp = esp;
    ctx->state = WIFI_STATE_INIT;
    ctx->prev_time = 0;
    ctx->connected = 0;
}

void wifiProcess(wifi_t *ctx) {
    if (ctx == NULL || ctx->esp == NULL) {
        return;
    }

    esp8266_Process(ctx->esp);

    if (ctx->state == WIFI_STATE_READY) {
        if (strstr(ctx->esp->rx_buf, "CLOSED") != NULL ||
            strstr(ctx->esp->rx_buf, "WIFI DISCONNECT") != NULL) {
            ctx->connected = 0;
            ctx->prev_time = bspMillis();
            ctx->state = WIFI_STATE_RETRY;
            ctx->esp->rx_buf[0] = '\0';
            ctx->esp->rx_len = 0;
        }
    }

    switch (ctx->state) {
        case WIFI_STATE_INIT:
            ctx->state = WIFI_STATE_AT_CHECK;
            break;
        case WIFI_STATE_AT_CHECK:
            ctx->state = (esp8266_TestAT(ctx->esp) == ESP8266_OK) ? WIFI_STATE_SET_MODE : WIFI_STATE_ERROR;
            break;
        case WIFI_STATE_SET_MODE:
            ctx->state = (esp8266_SetMode(ctx->esp, ESP8266_WIFI_MODE_STA) == ESP8266_OK) ? WIFI_STATE_JOIN_AP : WIFI_STATE_ERROR;
            break;
        case WIFI_STATE_JOIN_AP:
            ctx->state = (esp8266_JoinAP(ctx->esp, WIFI_SSID, WIFI_PASSWORD) == ESP8266_OK) ? WIFI_STATE_CONNECT_SERVER : WIFI_STATE_ERROR;
            break;
        case WIFI_STATE_CONNECT_SERVER:
            if (esp8266_StartTCP(ctx->esp, BLYNK_SERVER, BLYNK_PORT) == ESP8266_OK) {
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
            ctx->prev_time = bspMillis();
            ctx->state = WIFI_STATE_RETRY;
            break;
        case WIFI_STATE_RETRY:
            if ((bspMillis() - ctx->prev_time) >= WIFI_RETRY_INTERVAL_MS) {
                ctx->state = WIFI_STATE_INIT;
            }
            break;
        default:
            ctx->connected = 0;
            ctx->state = WIFI_STATE_INIT;
            break;
    }
}

uint8_t wifiIsConnected(wifi_t *ctx) {
    if (ctx == NULL) {
        return 0;
    }
    return ctx->connected;
}

void wifiSendElevatorStatus(wifi_t *ctx, uint8_t floor) {
  char tx_buf[256];

  if (ctx == NULL || ctx->esp == NULL || ctx->connected == 0U) {
    return;
  }

  // 전역 변수를 직접 건드리지 않고, 안전하게 복사본을 가져옵니다.
  bsp_elevator_input_t current_input;
  bspElevatorReadInput(&current_input);

  // 복사본에서 과전류 상태 확인
  uint8_t over_current_val = current_input.motor_over_current ? 1 : 0;

  snprintf(tx_buf, sizeof(tx_buf),
           "GET /external/api/update?token=%s&v0=%u&v5=%u HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Connection: keep-alive\r\n\r\n",
           BLYNK_TOKEN,
           (unsigned int)floor,
           over_current_val,
           BLYNK_SERVER);

  (void)esp8266_SendData(ctx->esp, tx_buf);
}

// 기존 정수 파싱 함수
static void wifiParsePinValue(const char *body, const char *key, int *out) {
    char *p;
    if (body == NULL || key == NULL || out == NULL) return;

    p = strstr(body, key);
    if (p == NULL) return;

    p = strstr(p, ":");
    if (p == NULL) return;

    while (*p != '\0' && (*p < '0' || *p > '9') && *p != ',' && *p != '}') {
        p++;
    }

    if (*p >= '0' && *p <= '9') {
        *out = *p - '0';
    }
}

// 💡 [추가] V4 임계값(소수점 포함 문자열) 파싱을 위한 함수
static void wifiParseFloatValue(const char *body, const char *key, float *out) {
    char *p;
    if (body == NULL || key == NULL || out == NULL) return;

    p = strstr(body, key);
    if (p == NULL) return;

    p = strstr(p, ":");
    if (p == NULL) return;

    // 숫자, 마이너스 부호, 소수점이 나올 때까지 탐색 (다른 JSON 필드로 넘어가지 않도록 안전장치)
    while (*p != '\0' && *p != ',' && *p != '}') {
        if ((*p >= '0' && *p <= '9') || *p == '-' || *p == '.') {
            *out = (float)atof(p);
            return;
        }
        p++;
    }
}

int wifiReceiveCommands(wifi_t *ctx, int *v1, int *v2, int *v3) {
    char tx_buf[160];
    uint32_t start_time;
    float v4_threshold = -1.0f; // 임계값 버퍼

    if (v1 == NULL || v2 == NULL || v3 == NULL) return -1;

    *v1 = -1; *v2 = -1; *v3 = -1;

    if (ctx == NULL || ctx->esp == NULL || ctx->connected == 0U) return -1;

    memset(ctx->esp->rx_buf, 0, sizeof(ctx->esp->rx_buf));
    ctx->esp->rx_len = 0;

    // 💡 [수정] V4(임계값 설정 핀) 값도 함께 받아오도록 요청
    snprintf(tx_buf, sizeof(tx_buf),
             "GET /external/api/get?token=%s&v1&v2&v3&v4 HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: keep-alive\r\n\r\n",
             BLYNK_TOKEN,
             BLYNK_SERVER);

    if (esp8266_SendData(ctx->esp, tx_buf) != ESP8266_OK) {
        return -1;
    }

    start_time = bspMillis();
    while ((bspMillis() - start_time) < 1200U) {
        char *body;

        esp8266_Process(ctx->esp);

        body = strstr(ctx->esp->rx_buf, "\r\n\r\n");
        if (body != NULL) {
            body += 4;

            // 기존 V1, V2, V3 버튼 처리
            wifiParsePinValue(body, "\"v1\"", v1);
            wifiParsePinValue(body, "\"v2\"", v2);
            wifiParsePinValue(body, "\"v3\"", v3);

            // 💡 [추가] V4 파싱 및 전류 임계값 설정
            wifiParseFloatValue(body, "\"v4\"", &v4_threshold);
            if (v4_threshold >= 0.0f) { // 유효한 값이 들어왔다면
                bspSetCurrentThreshold(v4_threshold);
            }

            if (*v1 != -1 || *v2 != -1 || *v3 != -1) {
                return 0;
            }
        }
        bspDelay(20);
    }
    return -1;
}

void wifiClearCommand(wifi_t *ctx, int pin) {
    char tx_buf[256];

    if (ctx == NULL || ctx->esp == NULL || ctx->connected == 0U) return;
    if (pin < 1 || pin > 3) return;

    snprintf(tx_buf, sizeof(tx_buf),
             "GET /external/api/update?token=%s&v%d=0 HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: keep-alive\r\n\r\n",
             BLYNK_TOKEN,
             pin,
             BLYNK_SERVER);

    (void)esp8266_SendData(ctx->esp, tx_buf);
    bspDelay(50);
}