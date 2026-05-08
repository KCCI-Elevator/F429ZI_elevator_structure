# 26/5/7
# MyApp refactor notes

## 목표

- Application 레이어에서 HAL, GPIO, CAN 전역 변수, ESP8266 전역 변수 직접 접근을 제거했습니다.
- BSP를 Application의 유일한 보드 인터페이스로 정리했습니다.
- Hardware 레이어는 HAL wrapper만 담당하도록 정리했습니다.
- CAN 수신/송신은 Hardware `my_can`에서 generic frame 단위로 처리하고, 엘리베이터 CAN 프로토콜 처리는 BSP에서 처리하도록 분리했습니다.
- Wi-Fi와 OLED, Keypad도 Application에서 직접 만지지 않고 BSP API를 통해 사용하도록 정리했습니다.

## 레이어 방향

Application -> BSP -> Driver -> Hardware -> HAL/CubeMX

## 주요 변경 파일

- Application/ap.c
  - HAL_GetTick, HAL_GPIO_WritePin, can_rx_flag, elevator_input 직접 접근 제거
  - Task 함수는 weak override 유지
  - StartElevatorTask, StartCanRXTask, StartKeypadTask, StartWifiTask가 BSP API만 사용

- Application/elevator.c
  - 전역 elevator_input 접근 제거
  - send_can_* 직접 호출 제거
  - 상태 변경에 필요한 출력은 BSP setter/API 사용

- BSP/bsp.c/h
  - elevator_input을 static으로 은닉
  - 입력 읽기, 상태 setter, CAN 프로토콜, OLED, keypad, Wi-Fi wrapper 제공

- Hardware/my_can.c/h
  - CAN HAL wrapper로 정리
  - CAN 프레임 송수신만 담당

- Hardware/my_gpio.c/h
  - read/write 시 매번 HAL_GPIO_Init하지 않도록 변경
  - gpioExtInitPull, gpioPinWrite, gpioPinRead 추가

- Driver/keypad.c/h
  - HAL 직접 호출 제거
  - my_gpio/hw wrapper 사용

- Driver/wifi.c/h
  - HAL_GetTick/osDelay 직접 사용 제거
  - hwMillis/hwDelay 사용
  - default context init API 추가

- Driver/oled_spi.c/h
  - 헤더에서 HAL 매크로 제거
  - my_gpio/hw wrapper 사용