#ifndef INA219_TEST_H
#define INA219_TEST_H

#include "main.h" // HAL 라이브러리 참조용

// INA219 I2C 주소 (A0, A1이 GND일 때 0x40, HAL에서는 좌측 시프트하여 0x80 사용)
#define INA219_ADDR (0x40 << 1)

// 레지스터 주소
#define REG_CONFIG      0x00
#define REG_SHUNTVOLT   0x01
#define REG_BUSVOLT     0x02
#define REG_POWER       0x03
#define REG_CURRENT     0x04
#define REG_CALIBRATION 0x05

// 함수 선언
HAL_StatusTypeDef INA219_Init(I2C_HandleTypeDef *hi2c);
float INA219_ReadCurrent_mA(I2C_HandleTypeDef *hi2c);
float INA219_ReadBusVoltage_V(I2C_HandleTypeDef *hi2c);
float INA219_ReadCurrent_Avg(I2C_HandleTypeDef *hi2c, uint8_t samples);
uint32_t INA219_GetI2cErrorCount(void);

#endif