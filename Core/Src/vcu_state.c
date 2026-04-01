/*
 * vcu_state.c
 *
 *  Created on: Mar 31, 2026
 *      Author: ishanchitale
 */

#include "vcu_state.h"

static VCU_STATE vcu_state = VCU_STATE_PRE_RTD;
bool ready_to_drive = false;
bool inverter_precharged = false;

void enterDriveMode() {
	HAL_NVIC_DisableIRQ(EXTI15_10_IRQn); // Disable RTD + PRCHG Buttons
	vcu_state = VCU_STATE_DRIVE;
}

void exitDriveMode() {
	ready_to_drive = false;
	inverter_precharged = false;
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	vcu_state = VCU_STATE_PRE_RTD;
}
