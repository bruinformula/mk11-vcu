/*
 * prchg.c
 *
 *  Created on: Jan 26, 2026
 *      Author: ishanchitale
 */

#include "prchg.h"
#include "fdcan.h"

FDCAN_TxHeaderTypeDef PRCHG_TxHeader;
uint8_t PRCHG_TxData[1];
bool inverter_precharged;

// TODO: A struct to hold the "status" of precharge w/ a couple of booleans and uint8_t...

void configurePrechargeMessage() {
	PRCHG_TxHeader.Identifier = BMS_PRCHG_TX_ID;  /* VCU TX ID */
	PRCHG_TxHeader.IdType = FDCAN_STANDARD_ID;
	PRCHG_TxHeader.TxFrameType = FDCAN_DATA_FRAME;
	PRCHG_TxHeader.DataLength = FDCAN_DLC_BYTES_8;
	PRCHG_TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	PRCHG_TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
	PRCHG_TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
	PRCHG_TxHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
	PRCHG_TxHeader.MessageMarker = 0;

	// TODO: Configure TxData accordingly..
	PRCHG_TxData[0] = 0x00;
}

void sendPrechargeRequest() {
	if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &PRCHG_TxHeader, PRCHG_TxData) != HAL_OK) {
		// HANDLE FAILURE
	}
	// OTHERWISE, SENT.... AWAIT RESPONSE..
	// 5 Second Timeout? How to implement...
}

void processPrechargeResponse() {
	inverter_precharged = true;
}
