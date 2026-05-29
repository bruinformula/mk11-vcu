/*
 * inverter.h
 *
 *  Created on: Jan 31, 2026
 *      Author: ishanchitale
 */

#ifndef INC_MOTOR_CONTROL_H_
#define INC_MOTOR_CONTROL_H_

#include "fdcan.h"
#include "stdbool.h"
#include "stdint.h"
#include "vcu_state.h"

// PEDAL_MODE IS EITHER TWO_APPS, ONLY_APPS1, OR ONLY APPS2
#define TWO_APPS 0
#define ONLY_APPS1 1
#define ONLY_APPS2 2
#define PEDAL_MODE ONLY_APPS2

#define APPS1_ADC_MAX_VAL 320
#define APPS1_ADC_MIN_VAL 1450

#define APPS2_ADC_MAX_VAL 540
#define APPS2_ADC_MIN_VAL 1950

#define BSE_ADC_MAX_VAL 1300
#define BSE_ADC_MIN_VAL 350

#define APPS_IMPLAUSIBILITY_PERCENT_DIFFERENCE 0.10
#define APPS_IMPLAUSIBILITY_TIMEOUT_MS 100
#define APPS_INFLECTION_PERCENT 0.05

#define BSE_IMPLAUSIBILITY_TIMEOUT_MS 100
#define BSE_ACTIVATED_ADC_THRESHOLD 1000

#define CROSSCHECK_IMPLAUSIBILITY_PERCENT_DIFFERENCE 0.25
#define CROSSCHECK_RESTORATION_APPS_PERCENT 0.05

#define RPM_TO_CARSPEED_CONVFACTOR                                             \
  (59.0f * 32.0f * 3.14159f * 60.0f) / (12.0f * 39370.1f)
#define MAX_TORQUE 120
#define MIN_TORQUE 0
#define REGEN_BASELINE_TORQUE 0
#define REGEN_MAX_TORQUE -30

extern uint16_t ADC_VAL[3];
extern float voltage_values[3];
extern float pedal_percents[3];
extern float requestedTorque;

typedef struct PlausibilityChecks {
  bool apps1_unplugged;
  bool apps2_unplugged;
  bool apps_plausible;
  bool bse_plausible;
  bool crosscheck_plausible;
} PlausibilityChecks;
extern PlausibilityChecks plausibility_checks;

typedef struct InverterDiagnostics {
  float inverter_rpm;
  float inverter_voltage;
  float inverter_carspeed;
} InverterDiagnostics;

void configureInverterMessage();
void calculateTorqueRequest();
void checkAPPS_Unplugged();
void checkAPPS_Plausibility();
void checkBSE_Plausibility();
void checkAPPS_BSE_Crosscheck();
void sendTorqueRequest(uint16_t requestedTorque, uint8_t forward,
                       uint8_t inverter_on);
void processInverter_Voltage();
void processInverter_RPM();
void resetPlausibilityChecks();

#endif /* INC_MOTOR_CONTROL_H_ */
