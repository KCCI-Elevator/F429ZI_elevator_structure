#include "my_can.h"

extern CAN_HandleTypeDef hcan1;
extern bsp_elevator_input_t elevator_input;

volatile uint8_t can_rx_flag = 0;
uint8_t can_rx_buf[8];
uint32_t can_rx_id;
uint8_t can_rx_len;

volatile uint8_t can_init_status = 0;
volatile uint32_t can_rx_callback_count = 0;

void blink_result(uint8_t is_ok) {
    int count = is_ok ? 2 : 1;
    int delay_ms = is_ok ? 200 : 600;

    for (int i = 0; i < count; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);   // LED ON
        HAL_Delay(delay_ms);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // LED OFF
        HAL_Delay(delay_ms);
    }
}

void can_init(void) {
    CAN_FilterTypeDef canFilter;

    canFilter.FilterBank = 0;
    canFilter.FilterMode = CAN_FILTERMODE_IDMASK;
    canFilter.FilterScale = CAN_FILTERSCALE_32BIT;
    canFilter.FilterIdHigh = 0x0000;
    canFilter.FilterIdLow = 0x0000;
    canFilter.FilterMaskIdHigh = 0x0000;
    canFilter.FilterMaskIdLow = 0x0000;
    canFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
    canFilter.FilterActivation = ENABLE;
    canFilter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(&hcan1, &canFilter) != HAL_OK) {
        can_init_status = 2;
        return;
    }

    HAL_StatusTypeDef status = HAL_CAN_Start(&hcan1);

    if (status != HAL_OK) {
        uint32_t can_error = hcan1.ErrorCode;
        can_init_status = 2;
        return;
    }

    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        can_init_status = 2;
        return;
    }

    can_init_status = 1;
}

void can_transmit(uint32_t id, uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;

    TxHeader.StdId = id;
    TxHeader.ExtId = 0x00;
    TxHeader.IDE   = CAN_ID_STD;
    TxHeader.RTR   = CAN_RTR_DATA;
    TxHeader.DLC   = len;
    TxHeader.TransmitGlobalTime = DISABLE;

    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0);

    if (HAL_CAN_AddTxMessage(&hcan1, &TxHeader, data, &TxMailbox) != HAL_OK) {
        // [수정] 송신 실패 시 while(1); 무한 대기를 삭제하고 바로 return
        return;
    }
}

// -----------------------------------------------------------------
// [송신부] F429 -> BluePill 상태 및 명령 하달
// -----------------------------------------------------------------

void send_car_status(void) {
    uint8_t tx_data[3] = {0};

    tx_data[0] = (uint8_t)elevator_input.curr_floor;
    tx_data[1] = (uint8_t)elevator_input.current_dir;
    tx_data[2] = (uint8_t)elevator_input.special_state;

    can_transmit(CAN_ID_STATUS, tx_data, 3);
}

void send_can_arrived_check(uint8_t floor) {
    uint8_t tx_data[1] = {floor};
    can_transmit(CAN_ID_ARRIVED_CHECK, tx_data, 1);
}

void send_can_door_cmd(uint8_t floor, uint8_t cmd, uint8_t dir) {
    uint8_t tx_data[3] = {floor, cmd, dir};
    can_transmit(CAN_ID_DOOR_CMD, tx_data, 3);
}


// -----------------------------------------------------------------
// [수신부] BluePill -> F429 데이터 수신 및 파싱
// -----------------------------------------------------------------

void process_can_rx(uint32_t rx_id, uint8_t *rx_data) {
    uint8_t floor_num = rx_data[0];

    switch (rx_id) {
      case CAN_ID_CALL_REQ:
        if (rx_data[1] == CAN_CALL_UP) {
          elevator_input.call_up[floor_num] = true;
        } else if (rx_data[1] == CAN_CALL_DOWN) {
          elevator_input.call_down[floor_num] = true;
        }
        break;

      case CAN_ID_CALL_CANCEL:
        if (rx_data[1] == CAN_CALL_UP) {
          elevator_input.call_up[floor_num] = false;
        } else if (rx_data[1] == CAN_CALL_DOWN) {
          elevator_input.call_down[floor_num] = false;
        }
        break;

      case CAN_ID_ARRIVED_ACK:
        if (floor_num == elevator_input.curr_floor) {
            elevator_input.arrived_ack = true;
        }
        break;

      default:
        break;
    }
}

/**
 * @brief CAN FIFO0 수신 인터럽트 콜백
 * [수정] 콜백 내 process_can_rx 호출 중복을 제거
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan1)
{
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t rx_data[8];

    can_rx_callback_count++;

    if (HAL_CAN_GetRxMessage(hcan1, CAN_RX_FIFO0, &RxHeader, rx_data) == HAL_OK)
    {
        if (RxHeader.IDE == CAN_ID_STD) {
            can_rx_id = RxHeader.StdId;
        } else {
            can_rx_id = RxHeader.ExtId;
        }

        can_rx_len = RxHeader.DLC;

        for (uint8_t i = 0; i < RxHeader.DLC; i++) {
            can_rx_buf[i] = rx_data[i];
        }

        for (uint8_t i = RxHeader.DLC; i < 8; i++) {
            can_rx_buf[i] = 0;
        }

        // [수정] 인터럽트 내에서 직접 파싱 방지. ap.c의 StartCanRXTask에서 처리함
        // process_can_rx(can_rx_id, can_rx_buf);

        can_rx_flag = 1;
    }
}

void test_can_tx(void) {
  uint8_t test_data[3] = {1, 0, 0};
  can_transmit(CAN_ID_STATUS, test_data, 3);
}