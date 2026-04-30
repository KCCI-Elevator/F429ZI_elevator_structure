#ifndef ELEVATOR_CONFIG_H_
#define ELEVATOR_CONFIG_H_

// 제어 주기 관련
#define CONTROL_PERIOD_MS    10u
#define CONTROL_DT_SEC       ((float)CONTROL_PERIOD_MS / 1000.f)

// 모터 하드웨어 관련 (BSP에서 참조)
#define MOTOR_MAX_DUTY       3000u
#define MOTOR_MIN_DUTY       700u

#define SCURVE_TRAVEL_MM    520.0f //이동 목표 거리.(52cm로 설정.)

// S-Curve 프로파일 관련 (Controller에서 참조)
#define SCURVE_MAX_VEL_MM_S  50.0f
#define SCURVE_MAX_ACC_MM_S2 35.0f
#define SCURVE_JERK_MM_S3    25.0f

// 물리적 위치 관련 (Unit Conv에서 참조)
#define MM_PER_FLOOR         200.0f  // 20cm

#define FLOOR_DWELL_MS      300u //도착 후 대기 시간.(300ms)

#endif