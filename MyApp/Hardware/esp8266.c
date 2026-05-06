//
// Created by hiimseoll on 26. 5. 6..
//

#include "esp8266.h"
#include "my_uart.h"

#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "main.h"
/*
 * ESP8266 AT ëª…ë ¹ ë“œë¼ì´ë²„
 *
 * êµ¬ì¡°:
 * esp8266.c  â†’ ESP-01 AT ëª…ë ¹ ì²˜ë¦¬
 * my_uart.c  â†’ ì‹¤ì œ UART ì†¡ìˆ˜ì‹
 *
 * ì£¼ì˜:
 * ì´ ì½”ë“œëŠ” 1ì°¨ êµ¬í˜„ìš© blocking ë°©ì‹ìž…ë‹ˆë‹¤.
 * esp8266_SendCmd() ë‚´ë¶€ì—ì„œ timeout ë™ì•ˆ ì‘ë‹µì„ ê¸°ë‹¤ë¦½ë‹ˆë‹¤.
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
 * UART ì¸í„°ëŸ½íŠ¸ ìˆ˜ì‹  ì‹œìž‘ì€ bsp.c ë˜ëŠ” my_uart_Init() ìª½ì—ì„œ
 * my_uart_StartRxIT()ë¡œ í•´ì£¼ëŠ” êµ¬ì¡°ë¥¼ ê¶Œìž¥í•©ë‹ˆë‹¤.
 */
void esp8266_Process(esp8266_t *ctx) {
    uint8_t data;

    if (ctx == NULL) return;

    /* my_uart.cì˜ ring bufferì— ìŒ“ì¸ ë°ì´í„°ë¥¼ esp8266 rx_bufë¡œ ì´ë™í•©ë‹ˆë‹¤*/
    while (uartAvailable(ctx->uart_ch) > 0) {
        data = uartRead(ctx->uart_ch);

        if (ctx->rx_len < (sizeof(ctx->rx_buf) - 1)) {
            ctx->rx_buf[ctx->rx_len++] = (char)data;
            ctx->rx_buf[ctx->rx_len] = '\0';
        }
        else
            esp8266_ClearRxBuffer(ctx);
        /*
         * ë²„í¼ê°€ ê½‰ ì°¨ë©´ ê°€ìž¥ ë‹¨ìˆœí•˜ê²Œ ì´ˆê¸°í™”í•©ë‹ˆë‹¤.
         * ë‚˜ì¤‘ì— +IPD ìˆ˜ì‹ ê¹Œì§€ ì œëŒ€ë¡œ ì²˜ë¦¬í•˜ë ¤ë©´ ring buffer ë°©ì‹ìœ¼ë¡œ ê°œì„ í•˜ë©´ ë©ë‹ˆë‹¤.
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

    ret = esp8266_SendCmd(ctx, "AT", "OK", ESP8266_DEFAULT_TIMEOUT);

    if (ret == ESP8266_OK)
        ctx->is_ready = 1;
    else
        ctx->is_ready = 0;

    return ret;
}

esp8266_result_t esp8266_Restart(esp8266_t *ctx) {
    esp8266_result_t ret;
    // AT+RST ì´í›„ì—ëŠ” ë³´í†µ ready ë¬¸ìžì—´ì´ ì¶œë ¥ë©ë‹ˆë‹¤.
    // íŽŒì›¨ì–´ ë²„ì „ì— ë”°ë¼ ì¶œë ¥ í˜•ì‹ì´ ì¡°ê¸ˆ ë‹¤ë¥¼ ìˆ˜ ìžˆìŠµë‹ˆë‹¤.
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
     * ì˜ˆ:
     * AT+CWJAP="SSID","PASSWORD"
     *
     * Wi-Fi ì—°ê²°ì€ ì‹œê°„ì´ ì˜¤ëž˜ ê±¸ë¦´ ìˆ˜ ìžˆìœ¼ë¯€ë¡œ timeoutì„ ê¸¸ê²Œ ì¤ë‹ˆë‹¤.
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
     * ë‹¨ì¼ ì—°ê²° ê¸°ì¤€:
     * AT+CIPSTART="TCP","192.168.0.10",5000
     */
    len = snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u", ip, (unsigned int)port);

    if (len < 0 || len >= (int)sizeof(cmd)) return ESP8266_ERROR;

    ret = esp8266_SendCmd(ctx, cmd, "OK", ESP8266_TCP_TIMEOUT);
    /*
     * ì´ë¯¸ ì—°ê²°ë˜ì–´ ìžˆëŠ” ìƒíƒœì—ì„œëŠ” ì¼ë¶€ íŽŒì›¨ì–´ê°€
     * ALREADY CONNECTEDë¥¼ ë°˜í™˜í•  ìˆ˜ ìžˆìŠµë‹ˆë‹¤.
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

    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u", (unsigned int)len);

    ret = esp8266_SendCmd(ctx, cmd, ">", ESP8266_SEND_TIMEOUT);

    if (ret != ESP8266_OK) return ret;

    esp8266_ClearRxBuffer(ctx);

    ret = esp8266_SendRaw(ctx, data);

    if (ret != ESP8266_OK) return ret;

    return esp8266_WaitResponse(ctx, "SEND OK", ESP8266_SEND_TIMEOUT);
}

esp8266_result_t esp8266_Close(esp8266_t *ctx) {
    esp8266_result_t ret;

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

    start_time = HAL_GetTick();

    while ((HAL_GetTick() - start_time) < timeout_ms) {
        esp8266_Process(ctx);

        if (esp8266_Contains(ctx, expect) != 0) return ESP8266_OK;
        // ëŒ€í‘œì ì¸ ì‹¤íŒ¨ ì‘ë‹µ ì²˜ë¦¬
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