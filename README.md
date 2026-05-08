# 26/5/7
# bug fix + current sensor

## 목표

- bug fix
- ina219(전류센서), emergency stop 구현

## 주요 변경 파일

- Application 
  - ap.c
    - MotorTask 동작 시 SafetyUpdate 추기적으로 호출
  - elevator.c
    - SENSOR_ACK_TIMEOUT_MS 비활성화(홀센서 검증 목적)
  - elevator_controller.c
    - Elevator_EmergencyStop 모터 비상 정지 기능 추가

- BSP
  - wifi.c
    - Emergency Stop 및 Current Limit Set 처리 로직 추가
  - bsp.c
    - bspSetCurrentThreshold added.
    - bspCheckOverCurrent added.

- Driver
  - ina219.c/.h 추가

- Hardware
  - my_can.c
    - Circular Queue 적용해 수신 시 Overwrite 문제 방지