#include "ina219_test.h"

#include "cmsis_os.h"

extern osMutexId_t i2cMutexHandle;//뮤텍스 적용
static uint16_t g_ina219_addr = INA219_ADDR;
static uint32_t g_ina219_i2c_error_count = 0;
static const uint16_t g_ina219_calibration = 4096;
static const uint16_t g_ina219_config = 0x399F;

// 16비트 데이터를 레지스터에 쓰는 함수
static void writeRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t value) {
   if(osMutexAcquire(i2cMutexHandle, 100) == osOK) {
    uint8_t data[3];
    data[0] = reg;
    data[1] = (value >> 8) & 0xFF; // 상위 바이트
    data[2] = value & 0xFF;        // 하위 바이트
    if (HAL_I2C_Master_Transmit(hi2c, g_ina219_addr, data, 3, 100) != HAL_OK) {
        g_ina219_i2c_error_count++;
    }
    osMutexRelease(i2cMutexHandle);
   } else {
    g_ina219_i2c_error_count++;
   }
}

// 16비트 데이터를 레지스터에서 읽어오는 함수
static HAL_StatusTypeDef readRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *out_val) {
    //통신 전 열쇠 빌리기
    if (out_val == NULL) {
        g_ina219_i2c_error_count++;
        return HAL_ERROR;
    }

    if (osMutexAcquire(i2cMutexHandle, 100) != osOK) {
        g_ina219_i2c_error_count++;
        return HAL_BUSY;
    }

    uint8_t data[2];
    HAL_StatusTypeDef tx = HAL_I2C_Master_Transmit(hi2c, g_ina219_addr, &reg, 1, 100);
    HAL_StatusTypeDef rx = HAL_OK;
    if (tx == HAL_OK) {
        rx = HAL_I2C_Master_Receive(hi2c, g_ina219_addr, data, 2, 100);
    }
    osMutexRelease(i2cMutexHandle); //열쇠 반납

    if (tx != HAL_OK || rx != HAL_OK) {
        g_ina219_i2c_error_count++;
        return HAL_ERROR;
    }

    *out_val = (uint16_t)((data[0] << 8) | data[1]);
    return HAL_OK;
}

// 초기화: 32V, 2A 범위 설정 (션트 저항 0.1옴 기준)
HAL_StatusTypeDef INA219_Init(I2C_HandleTypeDef *hi2c) {
    // 전원/버스 안정화 대기
    osDelay(20);

    // 1. 디바이스 응답 확인 (A0/A1 배선 차이를 고려해 0x40~0x4F 탐색)
    for (uint8_t addr7 = 0x40; addr7 <= 0x4F; addr7++) {
        uint16_t addr8 = (uint16_t)(addr7 << 1);
        if (HAL_I2C_IsDeviceReady(hi2c, addr8, 3, 100) == HAL_OK) {
            g_ina219_addr = addr8;
            break;
        }
    }

    if (HAL_I2C_IsDeviceReady(hi2c, g_ina219_addr, 3, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    // 2. Calibration 설정 (0.1옴 션트저항 기준 약 4096 설정 시 LSB가 0.1mA가 됨)
    // 3.2A 범위를 위해 보통 4096 사용
    writeRegister(hi2c, REG_CALIBRATION, g_ina219_calibration);

    // 3. Config 설정 (32V 범위, Gain /8, 12비트 샘플링)
    writeRegister(hi2c, REG_CONFIG, g_ina219_config);

    return HAL_OK;
}

// 전류 읽기 (mA 단위 반환)
float INA219_ReadCurrent_mA(I2C_HandleTypeDef *hi2c) {
    static int16_t last_valid_raw_current = 0;
    static uint16_t zero_read_count = 0;
    uint16_t raw = 0;
    if (readRegister(hi2c, REG_CURRENT, &raw) == HAL_OK) {
        // INA219가 리셋되면 calibration이 0으로 풀리고 current가 계속 0이 되므로 자동 복구
        if (raw == 0) {
            if (zero_read_count < 10U) {
                zero_read_count++;
            }
            if (zero_read_count >= 3U) {
                writeRegister(hi2c, REG_CALIBRATION, g_ina219_calibration);
                writeRegister(hi2c, REG_CONFIG, g_ina219_config);
            }
        } else {
            zero_read_count = 0;
        }
        last_valid_raw_current = (int16_t)raw;
    }
    // Calibration 값에 따라 계산 (보통 1 LSB = 0.1mA일 때 10.0으로 나눔)
    return (float)last_valid_raw_current / 10.0f;
}

// 버스 전압 읽기 (V 단위 반환)
float INA219_ReadBusVoltage_V(I2C_HandleTypeDef *hi2c) {
    static uint16_t last_valid_raw_bus = 0;
    uint16_t value = 0;
    if (readRegister(hi2c, REG_BUSVOLT, &value) == HAL_OK) {
        last_valid_raw_bus = value;
    } else {
        value = last_valid_raw_bus;
    }
    // 전압 값은 3번 비트부터 유효하며 1 LSB는 4mV임
    return (float)((value >> 3) * 4) / 1000.0f;
}

//루프를 돌며 데이터 수집, 평균내는 로직 구현.
float INA219_ReadCurrent_Avg(I2C_HandleTypeDef *hi2c, uint8_t samples) {
    float sum = 0;
    if (samples == 0) return 0.0f;

    for (uint8_t i = 0; i < samples; i++) {
        sum += INA219_ReadCurrent_mA(hi2c);
        osDelay(5); // 센서 샘플링 속도에 맞춘 짧은 지연
    }

    return sum / (float)samples;
}

uint32_t INA219_GetI2cErrorCount(void) {
    return g_ina219_i2c_error_count;
}