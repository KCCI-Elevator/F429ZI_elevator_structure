#ifndef MOTION_UNIT_CONV_H_
#define MOTION_UNIT_CONV_H_

#include "def.h"

// 거리 변환 상수
#define REF_PULSE_FOR_MM    11000.0f
#define REF_DISTANCE_MM        520.0f

// 층 변환 상수
#define MM_PER_FLOOR         200.0f  // 1층당 20cm
#define BASE_FLOOR_OFFSET      0.0f  // 1층의 시작 위치 (0mm)

// 함수 선언
float motionPulseToMM(float pulse);
float motionMMToPulse(float mm);

// 추가: 층 -> mm 변환
float motionFloorToMM(uint8_t floor);

#endif