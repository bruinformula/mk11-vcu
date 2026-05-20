/*
 * vcu_state.c
 *
 *  Created on: Mar 31, 2026
 *      Author: ishanchitale
 */

#include "vcu_state.h"

volatile VCU_STATE vcu_state = VCU_IDLE;
volatile bool rtd_button_pressed;
volatile bool prchg_button_pressed;

void resetVCU() {
	HAL_TIM_Base_Stop_IT(&htim1);
	HAL_TIM_Base_Stop_IT(&htim2);
	HAL_TIM_Base_Stop_IT(&htim3);
	HAL_ADC_Stop_DMA(&hadc3);

	precharge_state = PRECHARGE_IDLE;
	precharge_response_received = false;
	vcu_state = VCU_IDLE;
	rtd_button_pressed = false; // Clamp button state
	prchg_button_pressed = false; // Clamp button state
	resetPlausibilityChecks();
	requestedTorque = 0;

	for (uint8_t disable_frames_sent = 0;
			disable_frames_sent < 5 && HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) > 0;
			++disable_frames_sent) {
		sendTorqueRequest(0, 0, 0);
	}

	// TODO: CROSSCHECK WITH BMS, BSPD, IMD; THE HARD FAULT SIGNALS!
	// HAL_Delay might be blocking?
//	uint8_t stable_fault_reads = 0;
//	for (uint8_t i = 0; i < 10; ++i) {
//		bool bms_fault  = (HAL_GPIO_ReadPin(BMS_FAULT_GPIO_Port, BMS_FAULT_Pin) == GPIO_PIN_RESET);
//		bool bspd_fault = (HAL_GPIO_ReadPin(BSPD_FAULT_GPIO_Port, BSPD_FAULT_Pin) == GPIO_PIN_RESET);
//		bool imd_fault  = (HAL_GPIO_ReadPin(IMD_FAULT_GPIO_Port, IMD_FAULT_Pin) == GPIO_PIN_RESET);
//
//		if (bms_fault || bspd_fault || imd_fault) {
//			stable_fault_reads++;
//		} else {
//			stable_fault_reads = 0;
//		}
//
//		HAL_Delay(10);
//	}
//
//	if (stable_fault_reads >= 10) {
//		Error_Handler();
//	}

	HAL_ADC_Start_DMA(&hadc3, (uint32_t*) ADC_VAL, 3); // Start pedals
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn); // Re-enable RTD + PRCHG Buttons
}
