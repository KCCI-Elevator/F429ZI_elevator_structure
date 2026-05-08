#include "my_can.h"

#include <string.h>

extern CAN_HandleTypeDef hcan1;

static volatile uint8_t s_can_rx_flag = 0;
static can_frame_t s_can_rx_frame;

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

    if (s_can_rx_flag == 0U) {
        return false;
    }

    __disable_irq();
    memcpy(frame, &s_can_rx_frame, sizeof(can_frame_t));
    s_can_rx_flag = 0;
    __enable_irq();

    return true;
}

void canResetRx(void)
{
    __disable_irq();
    memset(&s_can_rx_frame, 0, sizeof(s_can_rx_frame));
    s_can_rx_flag = 0;
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

    can_rx_callback_count++;

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, data) != HAL_OK) {
        return;
    }

    __disable_irq();
    s_can_rx_frame.id = (header.IDE == CAN_ID_STD) ? header.StdId : header.ExtId;
    s_can_rx_frame.len = header.DLC > 8U ? 8U : header.DLC;
    memcpy(s_can_rx_frame.data, data, s_can_rx_frame.len);
    if (s_can_rx_frame.len < 8U) {
        memset(&s_can_rx_frame.data[s_can_rx_frame.len], 0, 8U - s_can_rx_frame.len);
    }
    s_can_rx_flag = 1;
    __enable_irq();
}
