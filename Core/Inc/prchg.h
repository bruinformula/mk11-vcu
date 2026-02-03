/*
 * prchg.h
 *
 *  Created on: Jan 26, 2026
 *      Author: ishanchitale
 */

#ifndef INC_PRCHG_H_
#define INC_PRCHG_H_

#include "stdbool.h"
#include "fdcan.h"
#include "gpio.h"

extern FDCAN_TxHeaderTypeDef PRCHG_TxHeader;
extern uint8_t PRCHG_TxData[1];
extern bool inverter_precharged;

void configurePrechargeMessage();
void sendPrechargeRequest();
void processPrechargeResponse();

#endif /* INC_PRCHG_H_ */
