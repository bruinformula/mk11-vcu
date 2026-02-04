/*
 * inverter.h
 *
 *  Created on: Jan 31, 2026
 *      Author: ishanchitale
 */

#ifndef INC_MOTOR_CONTROL_H_
#define INC_MOTOR_CONTROL_H_

#include "fdcan.h"
#include "stdint.h"

#define MAX_TORQUE 100
#define REGEN_MAX_TORQUE 100

extern FDCAN_TxHeaderTypeDef Inverter_TxHeader;
extern uint8_t Inverter_TxData[8];

void configureInverterMessage();
void calculateTorqueRequest();
void checkAPPS_Plausibility();
void checkAPPS_BSE_Crosscheck();
void sendTorqueRequest(int requestedTorque);
void stopMotor();

#endif /* INC_MOTOR_CONTROL_H_ */
