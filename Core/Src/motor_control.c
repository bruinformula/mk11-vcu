/*
 * motor_control.c
 *
 *  Created on: Jan 31, 2026
 *      Author: ishanchitale
 */

#include "motor_control.h"

FDCAN_TxHeaderTypeDef Inverter_TxHeader;
uint8_t Inverter_TxData[8];

void configureInverterMessage() {
	Inverter_TxHeader.IdType = FDCAN_STANDARD_ID;
	Inverter_TxHeader.TxFrameType = FDCAN_DATA_FRAME;
	Inverter_TxHeader.DataLength = FDCAN_DLC_BYTES_8;
	Inverter_TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	Inverter_TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
	Inverter_TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
	Inverter_TxHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
	Inverter_TxHeader.MessageMarker = 0;
}

void sendTorqueRequest(int requestedTorque_i) {
	uint8_t msg0 = (uint8_t)(requestedTorque_i & 0xFF);
	uint8_t msg1 = (uint8_t)((requestedTorque_i >> 8) & 0xFF);

	Inverter_TxHeader.Identifier = INVERTER_TORQUE_REQUEST_TX_ID;

	Inverter_TxData[0] = msg0;
	Inverter_TxData[1] = msg1;
	Inverter_TxData[2] = 0;
	Inverter_TxData[3] = 0;
	Inverter_TxData[4] = 1; // Forward
	Inverter_TxData[5] = 1; // Inverter On
	Inverter_TxData[6] = 0; // Default Torque Limits
	Inverter_TxData[7] = 0; // Default Torque Limits

	if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &Inverter_TxHeader, Inverter_TxData) != HAL_OK) {
		// HANDLE FAILURE!
	}

}

void stopMotor() {
	for (int i = 0; i < 5; ++i) {
		sendTorqueRequest(0);
	}
}
