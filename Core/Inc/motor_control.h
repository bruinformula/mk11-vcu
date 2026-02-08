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
#include "stdbool.h"

#define APPS1_ADC_MAX_VAL 2050
#define APPS1_ADC_MIN_VAL 60

#define APPS2_ADC_MAX_VAL 2800
#define APPS2_ADC_MIN_VAL 60

#define BSE_ADC_MAX_VAL 2800
#define BSE_ADC_MIN_VAL 50

#define APPS_IMPLAUSIBILITY_PERCENT_DIFFERENCE 0.10
#define APPS_IMPLAUSIBILITY_TIMEOUT_MS 100
#define APPS_INFLECTION_PERCENT 0.05

#define BSE_IMPLAUSIBILITY_TIMEOUT_MS 100
#define BSE_ACTIVATED_ADC_THRESHOLD 1500

#define CROSSCHECK_IMPLAUSIBILITY_PERCENT_DIFFERENCE 0.25
#define CROSSCHECK_RESTORATION_APPS_PERCENT 0.05

#define MAX_TORQUE 125
#define MIN_TORQUE 0
#define REGEN_BASELINE_TORQUE 0
#define REGEN_MAX_TORQUE -30

extern FDCAN_TxHeaderTypeDef Inverter_TxHeader;
extern uint8_t Inverter_TxData[8];
extern uint16_t ADC_VAL[3];
extern float voltage_values[3];
extern float pedal_percents[3];
extern float requestedTorque;

extern bool apps_plausible;
extern uint32_t millis_since_apps_implausible;
extern bool bse_plausible;
extern uint32_t millis_since_bse_implausible;
extern bool crosscheck_plausible;

void configureInverterMessage();
void calculateTorqueRequest();
void checkAPPS_Plausibility();
void checkBSE_Plausibility();
void checkAPPS_BSE_Crosscheck();
void sendTorqueRequest(int requestedTorque);
void stopMotor();

#endif /* INC_MOTOR_CONTROL_H_ */
