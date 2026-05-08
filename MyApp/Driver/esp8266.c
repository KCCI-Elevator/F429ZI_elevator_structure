#include "esp8266.h"
#include "hw.h"
#include "my_uart.h"

#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "main.h"
/*
 * ESP8266 AT 명령 드라이버
 *
 * 구조:
 * esp8266.c  → ESP-01 AT 명령 처리
 * my_uart.c  → 실제 UART 송수신
 *
 * 주의:
 * 이 코드는 1차 구현용 blocking 방식입니다.
 * esp8266_SendCmd() 내부에서 timeout 동안 응답을 기다립니다.
 */
#define ESP8266_CMD_BUF_SIZE 128
#define ESP8266_SEND_BUF_SIZE 64

#define ESP8266_DEFAULT_TIMEOUT 1000
#define ESP8266_JOIN_TIMEOUT 10000
#define ESP8266_TCP_TIMEOUT 5000
#define ESP8266_SEND_TIMEOUT 5000

// inner
static void esp8266_ClearRxBuffer(esp8266_t *ctx);
static void esp8266_FlushUartRx(esp8266_t *ctx);
static uint8_t esp8266_Contains(esp8266_t *ctx, const char *str);

static esp8266_result_t esp8266_WaitResponse(
    esp8266_t *ctx,
    const char *expect,
    uint32_t timeout_ms);

static esp8266_result_t esp8266_SendRaw(
    esp8266_t *ctx,
    const char *data);

#if 1
static void esp8266_Yield(void){
    if (osKernelGetState() == osKernelRunning) {
        osDelay(1);
    }
}
#endif

// function
void esp8266_Init(esp8266_t *ctx, uint8_t uart_ch) {
    if (ctx == NULL) return;

    ctx->uart_ch = uart_ch;
    ctx->rx_len = 0;
    ctx->is_ready = 0;
    ctx->is_connected = 0;

    memset(ctx->rx_buf, 0, sizeof(ctx->rx_buf));
}
/*
 * UART 인터럽트 수신 시작은 bsp.c 또는 my_uart_Init() 쪽에서
 * my_uart_StartRxIT()로 해주는 구조를 권장합니다.
 */
void esp8266_Process(esp8266_t *ctx) {
    uint8_t data;

    if (ctx == NULL) return;

    /* my_uart.c의 ring buffer에 쌓인 데이터를 esp8266 rx_buf로 이동합니다*/
    while (uartAvailable(ctx->uart_ch) > 0) {
        data = uartRead(ctx->uart_ch);

        if (ctx->rx_len < (sizeof(ctx->rx_buf) - 1)) {
            ctx->rx_buf[ctx->rx_len++] = (char)data;
            ctx->rx_buf[ctx->rx_len] = '\0';
        }
        else
            esp8266_ClearRxBuffer(ctx);
        /*
         * 버퍼가 꽉 차면 가장 단순하게 초기화합니다.
         * 나중에 +IPD 수신까지 제대로 처리하려면 ring buffer 방식으로 개선하면 됩니다.
         */
    }
}

esp8266_result_t esp8266_SendCmd(
    esp8266_t *ctx,
    const char *cmd,
    const char *expect,
    uint32_t timeout_ms) {
    esp8266_result_t ret;

    if (ctx == NULL || cmd == NULL || expect == NULL) return ESP8266_ERROR;

    esp8266_FlushUartRx(ctx);
    esp8266_ClearRxBuffer(ctx);

    ret = esp8266_SendRaw(ctx, cmd);
    if (ret != ESP8266_OK) return ret;

    ret = esp8266_SendRaw(ctx, "\r\n");
    if (ret != ESP8266_OK) return ret;

    return esp8266_WaitResponse(ctx, expect, timeout_ms);
}

esp8266_result_t esp8266_TestAT(esp8266_t *ctx) {
    esp8266_result_t ret;
    // 정상 응답: AT, OK
    ret = esp8266_SendCmd(ctx, "AT", "OK", ESP8266_DEFAULT_TIMEOUT);

    if (ret == ESP8266_OK)
        ctx->is_ready = 1;
    else
        ctx->is_ready = 0;

    return ret;
}

esp8266_result_t esp8266_Restart(esp8266_t *ctx) {
    esp8266_result_t ret;
    // AT+RST 이후에는 보통 ready 문자열이 출력됩니다.
    // 펌웨어 버전에 따라 출력 형식이 조금 다를 수 있습니다.
    ret = esp8266_SendCmd(ctx, "AT+RST", "ready", 5000);

    ctx->is_connected = 0;

    if (ret == ESP8266_OK)
        ctx->is_ready = 1;
    else
        ctx->is_ready = 0;

    return ret;
}

esp8266_result_t esp8266_SetMode(esp8266_t *ctx, esp8266_wifi_mode_t mode) {
    char cmd[ESP8266_CMD_BUF_SIZE];
    int len;

    if (mode != ESP8266_WIFI_MODE_STA &&
        mode != ESP8266_WIFI_MODE_AP &&
        mode != ESP8266_WIFI_MODE_STA_AP) {
        return ESP8266_ERROR;
    }
    /*
     * mode:
     * 1 = Station
     * 2 = SoftAP
     * 3 = Station + SoftAP
     */
    len = snprintf(cmd, sizeof(cmd), "AT+CWMODE=%d", mode);
    if (len < 0 || len >= (int)sizeof(cmd)) return ESP8266_ERROR;

    return esp8266_SendCmd(ctx, cmd, "OK", ESP8266_DEFAULT_TIMEOUT);
}

esp8266_result_t esp8266_JoinAP(
    esp8266_t *ctx,
    const char *ssid,
    const char *password) {
    char cmd[ESP8266_CMD_BUF_SIZE];
    int len;
    esp8266_result_t ret;

    if (ctx == NULL || ssid == NULL || password == NULL) return ESP8266_ERROR;
    /*
     * 예:
     * AT+CWJAP="SSID","PASSWORD"
     *
     * Wi-Fi 연결은 시간이 오래 걸릴 수 있으므로 timeout을 길게 줍니다.
     */
    len = snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, password);

    if (len < 0 || len >= (int)sizeof(cmd)) return ESP8266_ERROR;

    ret = esp8266_SendCmd(ctx, cmd, "OK", ESP8266_JOIN_TIMEOUT);

    if (ret == ESP8266_OK)
        ctx->is_connected = 1;
    else
        ctx->is_connected = 0;

    return ret;
}

esp8266_result_t esp8266_StartTCP(
    esp8266_t *ctx,
    const char *ip,
    uint16_t port) {
    char cmd[ESP8266_CMD_BUF_SIZE];
    int len;
    esp8266_result_t ret;

    if (ctx == NULL || ip == NULL) return ESP8266_ERROR;
    /*
     * 단일 연결 기준:
     * AT+CIPSTART="TCP","192.168.0.10",5000
     */
    len = snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u", ip, (unsigned int)port);

    if (len < 0 || len >= (int)sizeof(cmd)) return ESP8266_ERROR;

    ret = esp8266_SendCmd(ctx, cmd, "OK", ESP8266_TCP_TIMEOUT);
    /*
     * 이미 연결되어 있는 상태에서는 일부 펌웨어가
     * ALREADY CONNECTED를 반환할 수 있습니다.
     */
    if (ret != ESP8266_OK) {
        if (esp8266_Contains(ctx, "ALREADY CONNECTED") != 0) return ESP8266_OK;
    }

    return ret;
}

esp8266_result_t esp8266_SendData(esp8266_t *ctx, const char *data) {
    char cmd[ESP8266_SEND_BUF_SIZE];
    esp8266_result_t ret;
    int len;

    if (ctx == NULL || data == NULL) return ESP8266_ERROR;

    len = strlen(data);

    if (len == 0) return ESP8266_ERROR;
    /*
     * 단일 연결 기준:
     * AT+CIPSEND=<length>
     *
     * ESP8266이 '>' 프롬프트를 보내면 실제 데이터를 전송합니다.
     */
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u", (unsigned int)len);

    ret = esp8266_SendCmd(ctx, cmd, ">", ESP8266_SEND_TIMEOUT);

    if (ret != ESP8266_OK) return ret;

    esp8266_ClearRxBuffer(ctx);

    ret = esp8266_SendRaw(ctx, data);

    if (ret != ESP8266_OK) return ret;
    /* 정상 전송 시 보통 SEND OK가 출력됩니다. */
    return esp8266_WaitResponse(ctx, "SEND OK", ESP8266_SEND_TIMEOUT);
}

esp8266_result_t esp8266_Close(esp8266_t *ctx) {
    esp8266_result_t ret;
    /*
     * 단일 연결 기준:
     * AT+CIPCLOSE
     */
    ret = esp8266_SendCmd(ctx, "AT+CIPCLOSE", "OK", ESP8266_DEFAULT_TIMEOUT);

    if (ret == ESP8266_OK) ctx->is_connected = 0;

    return ret;
}

// inner function
static void esp8266_ClearRxBuffer(esp8266_t *ctx) {
    if (ctx == NULL) return;

    memset(ctx->rx_buf, 0, sizeof(ctx->rx_buf));
    ctx->rx_len = 0;
}

static void esp8266_FlushUartRx(esp8266_t *ctx) {
    if (ctx == NULL) return;
    // 이전 명령의 잔여 수신 데이터를 제거합니다.
    while (uartAvailable(ctx->uart_ch) > 0) {
        (void)uartRead(ctx->uart_ch);
    }
}

static uint8_t esp8266_Contains(esp8266_t *ctx, const char *str) {
    if (ctx == NULL || str == NULL) return 0;
    if (strstr(ctx->rx_buf, str) != NULL) return 1;

    return 0;
}

static esp8266_result_t esp8266_WaitResponse(
    esp8266_t *ctx,
    const char *expect,
    uint32_t timeout_ms) {
    uint32_t start_time;

    if (ctx == NULL || expect == NULL) return ESP8266_ERROR;

    start_time = hwMillis();

    while ((hwMillis() - start_time) < timeout_ms) {
        esp8266_Process(ctx);

        if (esp8266_Contains(ctx, expect) != 0) return ESP8266_OK;
        // 대표적인 실패 응답 처리
        if (esp8266_Contains(ctx, "ERROR") != 0) return ESP8266_ERROR;
        if (esp8266_Contains(ctx, "FAIL") != 0) return ESP8266_ERROR;
        if (esp8266_Contains(ctx, "busy") != 0) return ESP8266_BUSY;
    
        esp8266_Yield();
    }
    
    return ESP8266_TIMEOUT;
}

static esp8266_result_t esp8266_SendRaw(
    esp8266_t *ctx,
    const char *data) {
    uint32_t len;
    uint32_t written;

    if (ctx == NULL || data == NULL) return ESP8266_ERROR;

    len = strlen(data);

    if (len == 0) return ESP8266_ERROR;

    written = uartWrite(ctx->uart_ch, (uint8_t *)data, len);

    if (written != len) return ESP8266_ERROR;

    return ESP8266_OK;
}
