#include "hw.h"

void hwInit(void){
    // User Driver Init
}

uint32_t hwMillis(void){
    return HAL_GetTick();
}

void hwDelay(uint32_t delay_ms){
    HAL_Delay(delay_ms);
}

/* hw.c 파일의 맨 아래에 추가 */
#include <stdio.h>
#include "usart.h"

extern UART_HandleTypeDef huart3; // F429ZI의 VCP(가상 COM포트)는 보통 USART3입니다.

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
  // printf 호출 시 USART3를 통해 1바이트씩 전송되도록 연결
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}