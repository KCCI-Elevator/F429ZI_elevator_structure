#include "my_uart.h"
#include "main.h"
#include "usart.h"
#include <stdio.h>    // vsnprintf 사용을 위해 필요
#include <stdarg.h>   // va_list 사용을 위해 필요
#include <stdbool.h>  // bool, true, false 사용을 위해 필요
#include "cmsis_os2.h" // FreeRTOS/CMSIS-RTOS용

static osMessageQueueId_t uart_rx_q = NULL;
static osMutexId_t uart_tx_mutex = NULL;

#define TIMEOUT 200
#define APP_UART_HANDLE huart3
#define APP_UART_INSTANCE USART3

#define UART_RX_BUF_LENGTH 256

static uint8_t rx_data;

bool uartInit(void)
{

  if (uart_rx_q == NULL)
  {
    uart_rx_q = osMessageQueueNew(UART_RX_BUF_LENGTH, sizeof(uint8_t), NULL);
  }

  if (uart_tx_mutex == NULL)
  {
    uart_tx_mutex = osMutexNew(NULL);
  }

  bool ret = uartOpen(0, 115200);
  HAL_UART_Receive_IT(&APP_UART_HANDLE, &rx_data, 1);

  return ret;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

  if (huart->Instance == APP_UART_INSTANCE)
  {
    if (uart_rx_q != NULL)
    {
      osMessageQueuePut(uart_rx_q, &rx_data, 0, 0);
    }
    HAL_UART_Receive_IT(&APP_UART_HANDLE, &rx_data, 1);
  }
}

uint32_t uartAvailable(uint8_t ch)
{
  if (ch == 0 && uart_rx_q != NULL)
  {
    return osMessageQueueGetCount(uart_rx_q);
  }
  return 0;
}

uint8_t uartRead(uint8_t ch)
{
  uint8_t ret = 0;
  if (ch == 0 && uart_rx_q != NULL)
  {
    osMessageQueueGet(uart_rx_q, &ret, NULL, 0);
  }
  return ret;
}

bool uartReadBlock(uint8_t ch, uint8_t *p_data, uint32_t timeout)
{
  if (ch == 0 && uart_rx_q != NULL)
  {
    if (osMessageQueueGet(uart_rx_q, p_data, NULL, timeout) == osOK)
      return true;
  }
  return false;
}

bool uartOpen(uint8_t ch, uint32_t baudrate)
{

  if (APP_UART_HANDLE.Init.BaudRate != baudrate)
    APP_UART_HANDLE.Init.BaudRate = baudrate;

  if (HAL_UART_DeInit(&APP_UART_HANDLE) != HAL_OK)
    return false;

  if (HAL_UART_Init(&APP_UART_HANDLE) != HAL_OK)
    return false;

  return true;
}

bool uartClose(uint8_t ch)
{
  return true;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t len)
{
  if (uart_tx_mutex == NULL)
    return 0;

  osMutexAcquire(uart_tx_mutex, osWaitForever);

  if (HAL_UART_Transmit(&APP_UART_HANDLE, p_data, len, TIMEOUT) == HAL_OK)
  {
  }
  else
  {
    len = 0;
  }

  osMutexRelease(uart_tx_mutex);

  return len;
}

uint32_t uartPrintf(uint8_t ch, const char *fmt, ...)
{

  char buf[128];
  uint32_t len;
  va_list args;

  va_start(args, fmt);

  len = vsnprintf(buf, 128, fmt, args);

  va_end(args);
  return uartWrite(ch, (uint8_t *)buf, len);
}
