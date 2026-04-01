/*
 * vcu_state.h
 *
 *  Created on: Mar 31, 2026
 *      Author: ishanchitale
 */

#ifndef INC_VCU_STATE_H_
#define INC_VCU_STATE_H_

#include "stdbool.h"
#include "gpio.h"

typedef enum {
	VCU_STATE_PRE_RTD = 0,
	VCU_STATE_DRIVE,
} VCU_STATE;
extern bool ready_to_drive;
extern bool inverter_precharged;

void enterDriveMode();
void exitDriveMode();

#endif /* INC_VCU_STATE_H_ */
