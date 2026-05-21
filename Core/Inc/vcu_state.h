/*
 * vcu_state.h
 *
 *  Created on: Mar 31, 2026
 *      Author: ishanchitale
 */

#ifndef INC_VCU_STATE_H_
#define INC_VCU_STATE_H_

#include "stdbool.h"
#include "motor_control.h"
#include "prchg.h"
#include "gpio.h"
#include "adc.h"

typedef enum {
	VCU_IDLE = 0,
	VCU_PRECHARGED,
	VCU_DRIVE,
	VCU_IMD_FAULT,
	VCU_BMS_FAULT,
	VCU_BSPD_FAULT,
	VCU_CAN_FAULT
} VCU_STATE;
extern volatile VCU_STATE vcu_state;
extern volatile bool rtd_button_pressed;
extern volatile bool prchg_button_pressed;

void resetVCU();

#endif /* INC_VCU_STATE_H_ */
