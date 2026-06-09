/*
 * motor_control.c
 *
 *  Created on: Jan 31, 2026
 *      Author: ishanchitale
 */

#include "motor_control.h"
#include "vcu_state.h"

uint16_t ADC_VAL[3];
float voltage_values[3];
float pedal_percents[3];
float requestedTorque = 0.0f;

PlausibilityChecks plausibility_checks;
static uint32_t millis_since_apps_implausible;
static uint32_t millis_since_bse_implausible;

static FDCAN_TxHeaderTypeDef Inverter_TxHeader;
static uint8_t Inverter_TxData[8];
static InverterDiagnostics inverter_diagnostics;

static FDCAN_TxHeaderTypeDef VCU_Diagnostics_TxHeader;
static uint8_t VCU_Diagnostics_TxData[8];

// ------------------------
// PEDAL PROCCESSING LOGIC
// ------------------------

void calculateTorqueRequest() {
	if (vcu_state == VCU_DRIVE && inverter_diagnostics.inverter_voltage < 100.0f) {
		// The High-Voltage relays (AIRs) must have physically opened.
		// Trigger the actual lockout sequence safely!
		resetVCU();
		return;
	}

	float targetTorque = 0.0f;
	float apps_percent_average;

#if PEDAL_MODE == TWO_APPS
	apps_percent_average = (pedal_percents[0] + pedal_percents[1]) / 2;
#elif PEDAL_MODE == ONLY_APPS1
	apps_percent_average = pedal_percents[0];
#elif PEDAL_MODE == ONLY_APPS2
		apps_percent_average = pedal_percents[1];
#endif

	bool safety_torque_cut = false;
	if (plausibility_checks.apps1_invalid || plausibility_checks.apps2_invalid) {
		safety_torque_cut = true;
	}
	if (!plausibility_checks.crosscheck_plausible) {
		safety_torque_cut = true;
	}
	if (!plausibility_checks.apps_plausible && 
		(HAL_GetTick() - millis_since_apps_implausible) > APPS_IMPLAUSIBILITY_TIMEOUT_MS) {
		safety_torque_cut = true;
	}
	if (!plausibility_checks.bse_plausible && 
		(HAL_GetTick() - millis_since_bse_implausible) > BSE_IMPLAUSIBILITY_TIMEOUT_MS) {
		safety_torque_cut = true;
	}

  if (safety_torque_cut) {
	  targetTorque = 0.0f;
  } else if (apps_percent_average >= APPS_INFLECTION_PERCENT) {
	  // POSITIVE TORQUE REGION

	 targetTorque = ((float)(MAX_TORQUE - MIN_TORQUE)) *
			 (apps_percent_average - APPS_INFLECTION_PERCENT);

	 // GENERAL TORQUE CLAMPING
	 if (targetTorque >= MAX_TORQUE) {
		 targetTorque = MAX_TORQUE;
	 }

	 if (targetTorque <= MIN_TORQUE) {
		 targetTorque = MIN_TORQUE;
	 }
  } else {
	  // REGEN REGION

	  if (inverter_diagnostics.inverter_carspeed < 5.0f
			  || pedal_percents[2] > 0.05f) {
		  // Cancel region if car speed is low, or brake pedal is pressed (>5%).
		  // Commanding regen while brake pedal is pressed causes large phase currents and trips inverter.
		  targetTorque = 0.0f;
	  } else {
		  targetTorque = (REGEN_MAX_TORQUE - REGEN_BASELINE_TORQUE) *
		  			               ((APPS_INFLECTION_PERCENT - apps_percent_average) /
		  			                APPS_INFLECTION_PERCENT);
	  }
   }

   // LOW PASS FILTER AND SLEW RATE LIMITER
  static float filtered_target_torque = 0.0f;

  static uint32_t last_torque_calc_time = 0;
  uint32_t current_time = HAL_GetTick();

  // Initialize filter on startup!
  if (last_torque_calc_time == 0) {
	  last_torque_calc_time = current_time;
	  filtered_target_torque = targetTorque;
	  requestedTorque = filtered_target_torque;
	  return;
  }

  uint32_t dt_ms = current_time - last_torque_calc_time;
  last_torque_calc_time = current_time;

  if (dt_ms == 0) {
	  // In case this function runs twice within the same milisecond.
	  // requestedTorque retains old value!
	  return;
  }

  float dt = dt_ms/1000.0f;
  float alpha = dt / (RC_TIME_CONSTANT + dt);

  filtered_target_torque = (targetTorque * alpha) + (filtered_target_torque * (1.0f - alpha));
  float max_step = SLEW_RATE_LIMIT*dt;

  if (filtered_target_torque > requestedTorque + max_step) {
	  requestedTorque += max_step;
  } else if (filtered_target_torque < requestedTorque - max_step) {
	  requestedTorque -= max_step;
  } else {
	  requestedTorque = filtered_target_torque;
  }
}

void validateAPPS() {
// Note that as APPS1 and APPS2 are pressed, raw ADC value DECREASES.
// Handles 3 cases and cuts torque to 0 in case.
// 1. APPS1 or APPS2 are plugged in and not pressed; ADC Value will be larger than ADC_MIN_VAL.
// 2. APPS1 or APPS2 are plugged in and pressed beyond intended range of motion; ADC Value will be smaller than ADC_MAX_VAL.
// 3. APPS1 or APPS2 are unplugged and raw ADC value is ~ 70; this is smaller than ADC_MAX_VAL.
#define APPS_OVERSHOOT_BUFFER 250

#if PEDAL_MODE == TWO_APPS

    plausibility_checks.apps1_invalid = (ADC_VAL[0] > (APPS1_ADC_MIN_VAL + APPS_OVERSHOOT_BUFFER)) ||
    		(ADC_VAL[0] < (APPS1_ADC_MAX_VAL - APPS_OVERSHOOT_BUFFER));
    plausibility_checks.apps2_invalid = (ADC_VAL[1] > (APPS2_ADC_MIN_VAL + APPS_OVERSHOOT_BUFFER)) ||
        (ADC_VAL[1] < (APPS2_ADC_MAX_VAL - APPS_OVERSHOOT_BUFFER));

#elif PEDAL_MODE == ONLY_APPS1

    plausibility_checks.apps1_invalid = (ADC_VAL[0] > (APPS1_ADC_MIN_VAL + APPS_OVERSHOOT_BUFFER)) ||
        (ADC_VAL[0] < (APPS1_ADC_MAX_VAL - APPS_OVERSHOOT_BUFFER));
    plausibility_checks.apps2_invalid = false;

#elif PEDAL_MODE == ONLY_APPS2

    plausibility_checks.apps1_invalid = false;
    plausibility_checks.apps2_invalid = (ADC_VAL[1] > (APPS2_ADC_MIN_VAL + APPS_OVERSHOOT_BUFFER)) ||
        (ADC_VAL[1] < (APPS2_ADC_MAX_VAL - APPS_OVERSHOOT_BUFFER));

#endif

    if (plausibility_checks.apps1_invalid || plausibility_checks.apps2_invalid) {
        // Torque cut handled in calculateTorqueRequest
    }
}

void checkAPPS_Plausibility() {
#if PEDAL_MODE == TWO_APPS
  float pedal_travel_difference_percent =
      fabsf(pedal_percents[0] - pedal_percents[1]);
  bool apps_invalid = (pedal_travel_difference_percent >
                       APPS_IMPLAUSIBILITY_PERCENT_DIFFERENCE);

  if (plausibility_checks.apps_plausible == true && apps_invalid == true) {
    millis_since_apps_implausible = HAL_GetTick();
    plausibility_checks.apps_plausible = false;
  }

  if (plausibility_checks.apps_plausible == false) {
    if ((HAL_GetTick() - millis_since_apps_implausible) >
        APPS_IMPLAUSIBILITY_TIMEOUT_MS) {
      // Torque cut handled in calculateTorqueRequest
    }

    if (apps_invalid == false) {
      plausibility_checks.apps_plausible = true;
    }
  }
#else
  plausibility_checks.apps_plausible = true;
#endif
}

void checkBSE_Plausibility() {
  bool bse_invalid = (pedal_percents[2] > 1.0f || pedal_percents[2] < 0.0f);

  if (plausibility_checks.bse_plausible && bse_invalid) {
    millis_since_bse_implausible = HAL_GetTick();
    plausibility_checks.bse_plausible = false;
  }

  if (plausibility_checks.bse_plausible == false) {
    if ((HAL_GetTick() - millis_since_bse_implausible) >
        BSE_IMPLAUSIBILITY_TIMEOUT_MS) {
      // Torque cut handled in calculateTorqueRequest
    }

    if (bse_invalid == false) {
      plausibility_checks.bse_plausible = true;
    }
  }
}

void checkAPPS_BSE_Crosscheck() {
	float apps_percent_average;

#if PEDAL_MODE == TWO_APPS
	apps_percent_average = (pedal_percents[0] + pedal_percents[1]) / 2;
#elif PEDAL_MODE == ONLY_APPS1
	apps_percent_average = pedal_percents[0];
#elif PEDAL_MODE == ONLY_APPS2
	apps_percent_average = pedal_percents[1];
#endif

  if (plausibility_checks.crosscheck_plausible == true &&
      (apps_percent_average > CROSSCHECK_IMPLAUSIBILITY_PERCENT_DIFFERENCE) &&
      (ADC_VAL[2] > BSE_ACTIVATED_ADC_THRESHOLD)) {
    plausibility_checks.crosscheck_plausible = false;
  }

  if (plausibility_checks.crosscheck_plausible == false) {
    if (apps_percent_average <= CROSSCHECK_RESTORATION_APPS_PERCENT) {
      plausibility_checks.crosscheck_plausible = true; // CLEAR FAULT
    } else {
      // Torque cut handled in calculateTorqueRequest
    }
  }
}

// ----------------------
// CAN TX AND RX
// ----------------------

void configureInverterMessage() {
  Inverter_TxHeader.Identifier = INVERTER_TORQUE_REQUEST_TX_ID;
  Inverter_TxHeader.IdType = FDCAN_STANDARD_ID;
  Inverter_TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  Inverter_TxHeader.DataLength = FDCAN_DLC_BYTES_8;
  Inverter_TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  Inverter_TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  Inverter_TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  Inverter_TxHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
  Inverter_TxHeader.MessageMarker = 0;
}

void sendTorqueRequest(uint16_t requestedTorque_i, uint8_t forward,
                       uint8_t inverter_on) {
  uint8_t msg0 = (uint8_t)(requestedTorque_i & 0xFF);
  uint8_t msg1 = (uint8_t)((requestedTorque_i >> 8) & 0xFF);

  Inverter_TxData[0] = msg0;
  Inverter_TxData[1] = msg1;
  Inverter_TxData[2] = 0;
  Inverter_TxData[3] = 0;
  Inverter_TxData[4] = forward;     // Forward  (0), Backward (1)
  Inverter_TxData[5] = inverter_on; // Inverter Enable (1), Inverter Disable (0)
  Inverter_TxData[6] = 0;           // Default Torque Limits in EEPROM
  Inverter_TxData[7] = 0;           // Default Torque Limits in EEPROM

  HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &Inverter_TxHeader, Inverter_TxData);
}

void configureVCUDiagnosticsMessage() {
  VCU_Diagnostics_TxHeader.Identifier = VCU_DIAGNOSTICS_TX_ID;
  VCU_Diagnostics_TxHeader.IdType = FDCAN_STANDARD_ID;
  VCU_Diagnostics_TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  VCU_Diagnostics_TxHeader.DataLength = FDCAN_DLC_BYTES_8;
  VCU_Diagnostics_TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  VCU_Diagnostics_TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  VCU_Diagnostics_TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  VCU_Diagnostics_TxHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
  VCU_Diagnostics_TxHeader.MessageMarker = 0;
}

static uint32_t last_vcu_diag_send = 0;
void sendVCUDiagnostics() {
  if (HAL_GetTick() - last_vcu_diag_send < 50) return; // 20Hz Rate Limit

  int16_t carspeed = (int16_t)inverter_diagnostics.inverter_carspeed;
  int16_t requestedTorque_i = (int16_t)requestedTorque;
  int8_t apps1 = (int8_t)(pedal_percents[0] * 100.0f);
  int8_t apps2 = (int8_t)(pedal_percents[1] * 100.0f);
  int8_t bse = (int8_t)(pedal_percents[2] * 100.0f);

  uint8_t imd_fault = (HAL_GPIO_ReadPin(IMD_FAULT_GPIO_Port, IMD_FAULT_Pin) == GPIO_PIN_RESET) ? 1 : 0;
  uint8_t rtd_state = (vcu_state == VCU_DRIVE) ? 1 : 0;
  uint8_t precharge_relay = (precharge_state == PRECHARGE_SUCCESS) ? 1 : 0;
  uint8_t air_pos = (inverter_diagnostics.inverter_voltage > 100.0f) ? 1 : 0;
  uint8_t air_neg = (inverter_diagnostics.inverter_voltage > 100.0f) ? 1 : 0;
  uint8_t crosscheck = (!plausibility_checks.crosscheck_plausible) ? 1 : 0;
  uint8_t apps_plausible = (plausibility_checks.apps_plausible) ? 1 : 0;
  uint8_t looking_for_rtd = (vcu_state == VCU_PRECHARGED) ? 1 : 0;

  uint8_t flags = (imd_fault) |
                  (rtd_state << 1) |
                  (precharge_relay << 2) |
                  (air_pos << 3) |
                  (air_neg << 4) |
                  (crosscheck << 5) |
                  (apps_plausible << 6) |
                  (looking_for_rtd << 7);

  VCU_Diagnostics_TxData[0] = (uint8_t)(carspeed & 0xFF);
  VCU_Diagnostics_TxData[1] = (uint8_t)((carspeed >> 8) & 0xFF);
  VCU_Diagnostics_TxData[2] = (uint8_t)(requestedTorque_i & 0xFF);
  VCU_Diagnostics_TxData[3] = (uint8_t)((requestedTorque_i >> 8) & 0xFF);
  VCU_Diagnostics_TxData[4] = (uint8_t)apps1;
  VCU_Diagnostics_TxData[5] = (uint8_t)apps2;
  VCU_Diagnostics_TxData[6] = (uint8_t)bse;
  VCU_Diagnostics_TxData[7] = flags;

  if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) > 0) {
      HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &VCU_Diagnostics_TxHeader, VCU_Diagnostics_TxData);
      last_vcu_diag_send = HAL_GetTick();
  }
}

void processInverter_Voltage() {
  int16_t inverter_dc_volts_raw = (int16_t)((RxData1[1] << 8) | RxData1[0]);
  inverter_diagnostics.inverter_voltage = inverter_dc_volts_raw * 0.1f;
}

void processInverter_RPM() {
  inverter_diagnostics.inverter_rpm = fabsf(
		  (float)((int16_t)(RxData1[2] | (RxData1[3] << 8))));
  inverter_diagnostics.inverter_carspeed = fabsf(
		  (float)(inverter_diagnostics.inverter_rpm)
		  * RPM_TO_CARSPEED_CONVFACTOR);
}

void resetPlausibilityChecks() {
  plausibility_checks.apps1_invalid = false;
  plausibility_checks.apps2_invalid = false;
  plausibility_checks.apps_plausible = false;
  plausibility_checks.bse_plausible = false;
  plausibility_checks.crosscheck_plausible = false;
}
