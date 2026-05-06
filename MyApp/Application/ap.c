#include "ap.h"
#include "fan_control.h"
#include "i2c.h"
#include "ina219_test.h"
#include "load_monitor.h"
#include "my_uart.h"
#include "usart.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

static void dbg_uart_print(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, (uint16_t)strlen(msg), 100);
}

void apInit(void)
{
    uartInit();
    dbg_uart_print("DBG: apInit enter\r\n");
    Fan_Init();

    if (INA219_Init(&hi2c1) == HAL_OK)
    {
        dbg_uart_print("DBG: INA219 init OK\r\n");
        uartPrintf(0, "INA219 init OK\r\n");
    }
    else
    {
        dbg_uart_print("DBG: INA219 init FAIL\r\n");
        uartPrintf(0, "INA219 init FAIL\r\n");
    }

    Fan_Start();
    dbg_uart_print("DBG: MyApp started\r\n");
    uartPrintf(0, "MyApp started\r\n");
}

void apMain(void)
{
    float avg_current;
    int32_t value_x100;
    int32_t integer_part;
    int32_t frac_part;
    LoadStatus_t status;

    avg_current = INA219_ReadCurrent_Avg(&hi2c1, 10);
    status = Monitor_CheckWeight(avg_current);

    if (status == STATUS_OVERLOAD)
    {
        Fan_Stop();
        dbg_uart_print("DBG: OVERLOAD\r\n");
        uartPrintf(0, "!!! SYSTEM HALTED: OVERLOAD !!!\r\n");
        return;
    }

    value_x100 = (int32_t)(avg_current * 100.0f);
    integer_part = value_x100 / 100;
    frac_part = value_x100 % 100;
    if (frac_part < 0)
    {
        frac_part = -frac_part;
    }

    {
        char msg[64];
        int len = snprintf(msg, sizeof(msg), "DBG: Current Avg: %ld.%02ld mA\r\n", integer_part, frac_part);
        if (len > 0)
        {
            HAL_UART_Transmit(&huart2, (uint8_t *)msg, (uint16_t)len, 100);
        }
    }
    uartPrintf(0, "Current Avg: %ld.%02ld mA\r\n", integer_part, frac_part);
}