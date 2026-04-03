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
	precharge_state = PRECHARGE_IDLE;
	precharge_response_received = false;
	vcu_state = VCU_IDLE;
	rtd_button_pressed = false; // Clamp button state
	prchg_button_pressed = false; // Clamp button state
	resetPlausibilityChecks();
	requestedTorque = 0;
	HAL_ADC_Start_DMA(&hadc3, (uint32_t*) ADC_VAL, 3); // Start pedals
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn); // Re-enable RTD + PRCHG Buttons
}

void faultVCU() {
	HAL_ADC_Stop_DMA(&hadc3); // Stop pedals
	HAL_NVIC_DisableIRQ(EXTI15_10_IRQn); // Disable RTD + PRCHG Buttons
	HAL_TIM_Base_Stop_IT(&htim1); // Stop any running timers
	HAL_TIM_Base_Stop_IT(&htim2);
	HAL_TIM_Base_Stop_IT(&htim3);
	resetPlausibilityChecks();
	precharge_state = PRECHARGE_IDLE;
	precharge_response_received = false;
	vcu_state = VCU_FAULT;
	rtd_button_pressed = false; // Clamp button state
	prchg_button_pressed = false; // Clamp button state
	resetPlausibilityChecks();
	requestedTorque = 0;
}
