#include "ina219_test.h"

// 16비트 데이터를 레지스터에 쓰는 함수
static void writeRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t value) {
    uint8_t data[3];
    data[0] = reg;
    data[1] = (value >> 8) & 0xFF; // 상위 바이트
    data[2] = value & 0xFF;        // 하위 바이트
    HAL_I2C_Master_Transmit(hi2c, INA219_ADDR, data, 3, 100);
}

// 16비트 데이터를 레지스터에서 읽어오는 함수
static uint16_t readRegister(I2C_HandleTypeDef *hi2c, uint8_t reg) {
    uint8_t data[2];
    HAL_I2C_Master_Transmit(hi2c, INA219_ADDR, &reg, 1, 100);
    HAL_I2C_Master_Receive(hi2c, INA219_ADDR, data, 2, 100);
    return (uint16_t)((data[0] << 8) | data[1]);
}

// 초기화: 32V, 2A 범위 설정 (션트 저항 0.1옴 기준)
HAL_StatusTypeDef INA219_Init(I2C_HandleTypeDef *hi2c) {
    // 1. 디바이스 응답 확인
    if (HAL_I2C_IsDeviceReady(hi2c, INA219_ADDR, 3, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    // 2. Calibration 설정 (0.1옴 션트저항 기준 약 4096 설정 시 LSB가 0.1mA가 됨)
    // 3.2A 범위를 위해 보통 4096 사용
    writeRegister(hi2c, REG_CALIBRATION, 4096);

    // 3. Config 설정 (32V 범위, Gain /8, 12비트 샘플링)
    uint16_t config = 0x399F; 
    writeRegister(hi2c, REG_CONFIG, config);

    return HAL_OK;
}

// 전류 읽기 (mA 단위 반환)
float INA219_ReadCurrent_mA(I2C_HandleTypeDef *hi2c) {
    int16_t value = (int16_t)readRegister(hi2c, REG_CURRENT);
    // Calibration 값에 따라 계산 (보통 1 LSB = 0.1mA일 때 10.0으로 나눔)
    return (float)value / 10.0f; 
}

// 버스 전압 읽기 (V 단위 반환)
float INA219_ReadBusVoltage_V(I2C_HandleTypeDef *hi2c) {
    uint16_t value = readRegister(hi2c, REG_BUSVOLT);
    // 전압 값은 3번 비트부터 유효하며 1 LSB는 4mV임
    return (float)((value >> 3) * 4) / 1000.0f;
}

//루프를 돌며 데이터 수집, 평균내는 로직 구현.
float INA219_ReadCurrent_Avg(I2C_HandleTypeDef *hi2c, uint8_t samples) {
    float sum = 0;
    if (samples == 0) return 0.0f;

    for (uint8_t i = 0; i < samples; i++) {
        sum += INA219_ReadCurrent_mA(hi2c);
        HAL_Delay(5); // 센서 샘플링 속도에 맞춘 짧은 지연
    }

    return sum / (float)samples;
}