/*
 * prchg.c
 *
 *  Created on: Jan 26, 2026
 *      Author: ishanchitale
 */

#include "prchg.h"

static FDCAN_TxHeaderTypeDef PRCHG_TxHeader;
static uint8_t PRCHG_TxData[8];
static bool precharge_response_received = false;
static PrechargeState precharge_state = PRECHARGE_IDLE;

void configurePrechargeMessage() {
	PRCHG_TxHeader.Identifier = BMS_PRCHG_TX_ID;
	PRCHG_TxHeader.IdType = FDCAN_STANDARD_ID;
	PRCHG_TxHeader.TxFrameType = FDCAN_DATA_FRAME;
	PRCHG_TxHeader.DataLength = FDCAN_DLC_BYTES_8;
	PRCHG_TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	PRCHG_TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
	PRCHG_TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
	PRCHG_TxHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
	PRCHG_TxHeader.MessageMarker = 0;

	PRCHG_TxData[0] = 1;
}

void sendPrechargeRequest() {
	// PRECHARGE_IDLE and PRECHARGE_FAILURE are allowed "attempt" states
	if (precharge_state != PRECHARGE_IDLE && precharge_state != PRECHARGE_FAILURE) return;
	configurePrechargeMessage();
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &PRCHG_TxHeader, PRCHG_TxData);
	precharge_response_received = false;
    precharge_state = PRECHARGE_WAITING;
    HAL_TIM_Base_Start_IT(&htim3); // START PRECHARGE TIMER (6 SECONDS)
}

void processPrechargeResponse() {
	if (precharge_state != PRECHARGE_WAITING) return;
	precharge_response_received = true;

	if (RxData1[0] == 1) {
		inverter_precharged = true;
		precharge_state = PRECHARGE_SUCCESS;
	} else {
		inverter_precharged = false;
		precharge_state = PRECHARGE_FAILURE;
	}
}

void checkPrechargeStatus() {
	if (precharge_response_received == false) {
		inverter_precharged = false;
		precharge_state = PRECHARGE_TIMEOUT;
	}
}
