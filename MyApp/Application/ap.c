#include "ap.h"
#include "bsp.h"
#include "elevator.h"
#include "elevator_controller.h"
#include "ina219.h"
#include "cmsis_os2.h"
#include "i2c.h"
#include "safety_monitor.h"

static elevator_t s_elevator;
static volatile bool s_app_ready = false;

static uint8_t s_last_sent_floor = 0xFF;
static uint8_t s_last_sent_dir = 0xFF;
static uint8_t s_last_sent_state = 0xFF;
static uint32_t s_last_heartbeat_tick = 0;

static bool apShouldSendStatus(uint32_t now, const bsp_elevator_input_t *input)
{
    bool is_changed;
    bool is_heartbeat;

    if (input == NULL) {
        return false;
    }

    is_changed = (input->curr_floor != s_last_sent_floor) ||
                 ((uint8_t)input->current_dir != s_last_sent_dir) ||
                 ((uint8_t)input->special_state != s_last_sent_state);

    is_heartbeat = (now - s_last_heartbeat_tick >= 5000U);

    return is_changed || is_heartbeat;
}

static void apMarkStatusSent(uint32_t now, const bsp_elevator_input_t *input)
{
    if (input == NULL) {
        return;
    }

    s_last_sent_floor = input->curr_floor;
    s_last_sent_dir = (uint8_t)input->current_dir;
    s_last_sent_state = (uint8_t)input->special_state;
    s_last_heartbeat_tick = now;
}

void StartElevatorTask(void *argument)
{
    uint32_t prev_time;
    uint32_t oled_prev_time;

    (void)argument;

    bspInit();
    bspUiInit();
    (void)bspCanInit();
    Elevator_Controller_Init();
    elevatorInit(&s_elevator);

    bspDelay(100);
    bspCanSendTest();
    bspDelay(100);
    (void)bspCanSendStatus();

    prev_time = bspMillis();
    oled_prev_time = prev_time;
    s_app_ready = true;

    while (1) {
        uint32_t now = bspMillis();

        if (now - prev_time >= 10U) {
            bsp_elevator_input_t input;

            prev_time = now;

            bspUpdate();
            elevatorUpdate(&s_elevator, now);
            bspElevatorReadInput(&input);

            if (apShouldSendStatus(now, &input)) {
                if (input.curr_floor != 0U || input.special_state == BSP_STATE_MOVING) {
                    (void)bspCanSendStatus();
                    apMarkStatusSent(now, &input);
                }
            }
        }

        if (now - oled_prev_time >= 50U) {
            oled_prev_time = now;
            bspUiUpdate();
        }

        bspDelay(1);
    }
}

void StartDefaultTask(void *argument)
{
    (void)argument;

    while (1) {
        osDelay(1000);
    }
}

void motorTask(void *argument)
{
    Safety_Init();
    uint32_t tick_count = osKernelGetTickCount();

    while (s_app_ready == false) osDelay(1);

    while (1) {
        // 1. 센서 데이터 취득
        float current = INA219_ReadCurrent_mA(&hi2c2);

        // 2. 안전 감시 (이상 발생 시 내부에서 EmergencyStop 호출)
        Safety_Update(current);

        // 3. 제어기 업데이트 (내부에서 Safety 상태 확인 후 구동 결정)
        Elevator_Controller_Update();

        tick_count += 10U;
        osDelayUntil(tick_count);
    }
}

void StartCanRXTask(void *argument)
{
    (void)argument;

    while (s_app_ready == false) {
        osDelay(1);
    }

    while (1) {
        (void)bspCanProcessRx();
        osDelay(10);
    }
}

void StartKeypadTask(void *argument)
{
    char last_processed_key = 0;

    (void)argument;

    while (s_app_ready == false) {
        osDelay(1);
    }

    while (1) {
        char key = bspKeypadGetKey();

        if (key != 0 && key != last_processed_key) {
            if (key >= '1' && key <= '3') {
                bspToggleCarCall((uint8_t)(key - '0'));
            }
            else if (key == 'A') {
                /* Door open request hook can be added here. */
            }
            else if (key == 'B') {
                /* Door close request hook can be added here. */
            }
        }

        last_processed_key = key;
        osDelay(20);
    }
}

void StartWifiTask(void *argument)
{
    (void)argument;

    while (s_app_ready == false) {
        osDelay(1);
    }

    if (bspWifiInit() == false) {
        while (1) {
            osDelay(1000);
        }
    }

    while (1) {
        bspWifiProcess();
        osDelay(10);
    }
}
