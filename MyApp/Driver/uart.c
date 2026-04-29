#include "uart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// USART3 핸들러로 변경
extern UART_HandleTypeDef huart3; 

static osMessageQueueId_t uart_rx_q = NULL;
static osMutexId_t uart_tx_mutex = NULL;

#define TIMEOUT 200
#define UART_RX_BUF_LENGTH 256

static uint8_t rx_data;

bool uartInit(void) {
  if (uart_rx_q == NULL) {
    uart_rx_q = osMessageQueueNew(UART_RX_BUF_LENGTH, sizeof(uint8_t), NULL);
  }

  if(uart_tx_mutex == NULL) {
    uart_tx_mutex = osMutexNew(NULL);
  }

  bool ret = uartOpen(0, 115200); // PC 통신 표준인 115200 권장
  HAL_UART_Receive_IT(&huart3, &rx_data, 1);

  return ret;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3) { // USART3으로 변경
    if(uart_rx_q != NULL){
      osMessageQueuePut(uart_rx_q, &rx_data, 0, 0);
    }
    HAL_UART_Receive_IT(&huart3, &rx_data, 1);
  }
}

uint32_t uartAvailable(uint8_t ch) {
  if(ch==0 && uart_rx_q != NULL){
    return osMessageQueueGetCount(uart_rx_q);
  }
  return 0;
}

bool uartOpen(uint8_t ch, uint32_t baudrate) {
  if (huart3.Init.BaudRate != baudrate)
    huart3.Init.BaudRate = baudrate;

  if (HAL_UART_DeInit(&huart3) != HAL_OK)
    return false;

  if (HAL_UART_Init(&huart3) != HAL_OK)
    return false;

  return true;
}

bool uartClose(uint8_t ch) { return true; }

uint8_t uartRead(uint8_t ch) {
  uint8_t ret = 0;
  if(ch==0 && uart_rx_q != NULL){
    osMessageQueueGet(uart_rx_q, &ret, NULL, 0);
  }
  return ret;
}

bool uartReadBlock(uint8_t ch, uint8_t *p_data, uint32_t timeout) {
  if(ch==0 && uart_rx_q != NULL) {
    if(osMessageQueueGet(uart_rx_q, p_data, NULL, timeout) == osOK)
      return true;
  }
  return false;
}

// uart.c 내부의 uartWriteBlock 예시
bool uartWriteBlock(uint8_t ch, uint8_t *p_data, uint32_t Len) {
  // TIMEOUT 매크로를 사용하여 수정
  if (HAL_UART_Transmit(&huart3, p_data, Len, TIMEOUT) == HAL_OK) { 
    return true;
  }
  return false;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t Len) {
  if (uart_tx_mutex == NULL) return 0;
  
  osMutexAcquire(uart_tx_mutex, osWaitForever);
  
  if (HAL_UART_Transmit(&huart3, p_data, Len, TIMEOUT) == HAL_OK){ 
    osMutexRelease(uart_tx_mutex);
  }
  else{
    osMutexRelease(uart_tx_mutex);
  }
  return Len;
}

uint32_t uartPrintf(uint8_t ch, const char *fmt, ...) {
  char buf[128];
  uint32_t len;
  va_list args;

  va_start(args, fmt);
  len = vsnprintf(buf, 128, fmt, args);
  va_end(args);

  return uartWrite(ch, (uint8_t *)buf, len);
}