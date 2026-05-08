#include "my_can.h"
#include <string.h>

// 수신 큐 사이즈 정의 (필요에 따라 조절)
#define CAN_RX_QUEUE_SIZE 16U

extern CAN_HandleTypeDef hcan1;

// 원형 큐 변수 선언
static can_frame_t s_can_rx_queue[CAN_RX_QUEUE_SIZE];
static volatile uint8_t s_can_rx_head = 0;
static volatile uint8_t s_can_rx_tail = 0;

volatile uint8_t can_init_status = 0;
volatile uint32_t can_rx_callback_count = 0;

bool canInit(void)
{
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK) {
        can_init_status = 2;
        return false;
    }

    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        can_init_status = 2;
        return false;
    }

    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        can_init_status = 2;
        return false;
    }

    can_init_status = 1;
    return true;
}

bool canTransmit(uint32_t id, const uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox;
    uint32_t start_tick;

    if (data == NULL || len > 8U) {
        return false;
    }

    header.StdId = id;
    header.ExtId = 0x00;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = len;
    header.TransmitGlobalTime = DISABLE;

    start_tick = HAL_GetTick();
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0U) {
        if ((HAL_GetTick() - start_tick) > 10U) {
            return false;
        }
    }

    return HAL_CAN_AddTxMessage(&hcan1, &header, (uint8_t *)data, &mailbox) == HAL_OK;
}

bool canReceive(can_frame_t *frame)
{
    if (frame == NULL) {
        return false;
    }

    // 큐가 비어있는지 확인
    if (s_can_rx_head == s_can_rx_tail) {
        return false;
    }

    __disable_irq();

    // 큐에서 데이터 읽기 (Pop)
    memcpy(frame, &s_can_rx_queue[s_can_rx_tail], sizeof(can_frame_t));
    s_can_rx_tail = (s_can_rx_tail + 1U) % CAN_RX_QUEUE_SIZE;

    __enable_irq();

    return true;
}

void canResetRx(void)
{
    __disable_irq();

    // 큐 인덱스 및 버퍼 초기화
    s_can_rx_head = 0;
    s_can_rx_tail = 0;
    memset(s_can_rx_queue, 0, sizeof(s_can_rx_queue));

    __enable_irq();
}

void canTestTx(void)
{
    const uint8_t data[3] = {1, 0, 0};
    (void)canTransmit(CAN_ID_STATUS, data, 3);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef header = {0};
    uint8_t data[8] = {0};

    if (hcan == NULL || hcan->Instance != hcan1.Instance) {
        return;
    }

    // 하드웨어 FIFO에 메시지가 남아있는 동안 모두 읽어 큐에 저장
    while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0U) {

        can_rx_callback_count++;

        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, data) != HAL_OK) {
            continue;
        }

        uint8_t next_head = (s_can_rx_head + 1U) % CAN_RX_QUEUE_SIZE;

        // 큐 오버플로우 방지 (가득 차지 않았을 때만 저장)
        if (next_head != s_can_rx_tail) {
            s_can_rx_queue[s_can_rx_head].id = (header.IDE == CAN_ID_STD) ? header.StdId : header.ExtId;
            s_can_rx_queue[s_can_rx_head].len = (header.DLC > 8U) ? 8U : header.DLC;

            memcpy(s_can_rx_queue[s_can_rx_head].data, data, s_can_rx_queue[s_can_rx_head].len);

            if (s_can_rx_queue[s_can_rx_head].len < 8U) {
                memset(&s_can_rx_queue[s_can_rx_head].data[s_can_rx_queue[s_can_rx_head].len],
                       0,
                       8U - s_can_rx_queue[s_can_rx_head].len);
            }

            s_can_rx_head = next_head;
        }
    }
}