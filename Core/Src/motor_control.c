/*
 * motor_control.c
 *
 *  Created on: Jan 31, 2026
 *      Author: ishanchitale
 */

#include "motor_control.h"

static FDCAN_TxHeaderTypeDef Inverter_TxHeader;
static uint8_t Inverter_TxData[8];

uint16_t ADC_VAL[3];
float voltage_values[3];
float pedal_percents[3];
float requestedTorque;

static InverterDiagnostics inverter_diagnostics;

PlausibilityChecks plausibility_checks;
static uint32_t millis_since_apps_implausible;
static uint32_t millis_since_bse_implausible;

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

void calculateTorqueRequest() {
	float apps_percent_average;

#if PEDAL_MODE == TWO_APPS
	apps_percent_average = (pedal_percents[0] + pedal_percents[1]) / 2;
#elif PEDAL_MODE == ONLY_APPS1
	apps_percent_average = pedal_percents[0];
#elif PEDAL_MODE == ONLY_APPS2
		apps_percent_average = pedal_percents[1];
#endif

  if (apps_percent_average >= APPS_INFLECTION_PERCENT) {
    requestedTorque = ((float)(MAX_TORQUE - MIN_TORQUE)) *
                      (apps_percent_average - APPS_INFLECTION_PERCENT);

    if (requestedTorque >= MAX_TORQUE) {
      requestedTorque = MAX_TORQUE;
    }
  } else {
    if (inverter_diagnostics.inverter_carspeed < 5.0f) {
      requestedTorque = 0;
    } else {
      requestedTorque = (REGEN_MAX_TORQUE - REGEN_BASELINE_TORQUE) *
                        ((APPS_INFLECTION_PERCENT - apps_percent_average) /
                         APPS_INFLECTION_PERCENT);
    }
  }
}

void checkAPPS_Unplugged() {
// Unplugged state is detected if ADC values are way outside expected mechanical ranges
#define APPS_OVERSHOOT_BUFFER 250

#if PEDAL_MODE == TWO_APPS

    plausibility_checks.apps1_unplugged = (ADC_VAL[0] > (APPS1_ADC_MIN_VAL + APPS_OVERSHOOT_BUFFER)) ||
    		(ADC_VAL[0] < (APPS1_ADC_MAX_VAL - APPS_OVERSHOOT_BUFFER));
    plausibility_checks.apps2_unplugged = (ADC_VAL[1] > (APPS2_ADC_MIN_VAL + APPS_OVERSHOOT_BUFFER)) ||
        (ADC_VAL[1] < (APPS2_ADC_MAX_VAL - APPS_OVERSHOOT_BUFFER));

#elif PEDAL_MODE == ONLY_APPS1

    plausibility_checks.apps1_unplugged = (ADC_VAL[0] > (APPS1_ADC_MIN_VAL + APPS_OVERSHOOT_BUFFER)) ||
        (ADC_VAL[0] < (APPS1_ADC_MAX_VAL - APPS_OVERSHOOT_BUFFER));
    plausibility_checks.apps2_unplugged = false;

#elif PEDAL_MODE == ONLY_APPS2

    plausibility_checks.apps1_unplugged = false;
    plausibility_checks.apps2_unplugged = (ADC_VAL[1] > (APPS2_ADC_MIN_VAL + APPS_OVERSHOOT_BUFFER)) ||
        (ADC_VAL[1] < (APPS2_ADC_MAX_VAL - APPS_OVERSHOOT_BUFFER));

#endif

    if (plausibility_checks.apps1_unplugged || plausibility_checks.apps2_unplugged) {
        requestedTorque = 0;
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
      requestedTorque = 0;
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
      requestedTorque = 0;
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
    requestedTorque = 0;
  }

  if (plausibility_checks.crosscheck_plausible == false) {
    if (apps_percent_average <= CROSSCHECK_RESTORATION_APPS_PERCENT) {
      plausibility_checks.crosscheck_plausible = true; // CLEAR FAULT
    } else {
      requestedTorque = 0; // KEEP MOTOR SHUT DOWN
    }
  }
}

static HAL_StatusTypeDef TR_CAN_Debug;
static int torque_requests_sent;
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

  TR_CAN_Debug = HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &Inverter_TxHeader,
                                               Inverter_TxData);
  torque_requests_sent++;
}

void processInverter_Voltage() {
  int16_t inverter_dc_volts_raw = (int16_t)((RxData1[1] << 8) | RxData1[0]);
  inverter_diagnostics.inverter_voltage = inverter_dc_volts_raw * 0.1f;
}

void processInverter_RPM() {
  inverter_diagnostics.inverter_rpm =
      (float)((int16_t)(RxData1[2] | (RxData1[3] << 8)));
  inverter_diagnostics.inverter_carspeed =
      (float)(inverter_diagnostics.inverter_rpm) * RPM_TO_CARSPEED_CONVFACTOR;
}

void resetPlausibilityChecks() {
  plausibility_checks.apps1_unplugged = false;
  plausibility_checks.apps2_unplugged = false;
  plausibility_checks.apps_plausible = false;
  plausibility_checks.bse_plausible = false;
  plausibility_checks.crosscheck_plausible = false;
}
