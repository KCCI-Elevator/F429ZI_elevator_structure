#include "motion_unit_conv.h"

/**
 * 엔코더 펄스를 mm 거리로 변환
 */
float motionPulseToMM(float pulse) {
    return pulse * (REF_DISTANCE_MM / REF_PULSE_FOR_MM);
}

/**
 * mm 거리를 엔코더 펄스로 변환
 */
float motionMMToPulse(float mm) {
    return mm * (REF_PULSE_FOR_MM / REF_DISTANCE_MM);
}

/**
 * 층 번호를 mm 단위의 절대 위치로 변환
 * floor 층 번호 (1, 2, 3...)
 * 1층 기준으로 떨어진 거리(mm)
 * 예: 1층 -> 0mm, 2층 -> 200mm, 3층 -> 400mm
 */
float motionFloorToMM(uint8_t floor) {
    if (floor < 1) return 0.0f;
    return (float)(floor - 1) * MM_PER_FLOOR;
}