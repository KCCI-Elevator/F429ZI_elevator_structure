#include "elevator_io.h"
#include "uart.h"
#include "bsp.h" // 시리얼 읽기용
#include <stdio.h>

void Elevator_IO_Update(elevator_t *ctx) {
    
    //0번 채널에 데이터가 있는지 확인(Non-blocking)

    if (uartAvailable(0)>0){
        uint8_t rx_data = uartRead(0);

        /* 어떤 키를 눌러도 시리얼로 에코 (디버깅·확인용) */
        uartPrintf(0, "RX: %c (0x%02X)\r\n", (char)rx_data, rx_data);

        //1층(0x31), 2층(0x32), 3층(0x33) 확인
        if(rx_data >= '1' && rx_data <= '3') {
            uint8_t floor = rx_data - '0';
                ctx->req_mask |= FLOOR_BIT(floor);
              //UART로 피드백 송신
                uartPrintf(0,"UART Command: Floor %d received\r\n", floor);
            }
        }
    }
