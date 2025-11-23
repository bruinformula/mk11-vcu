/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BSE_3V3_Pin GPIO_PIN_8
#define BSE_3V3_GPIO_Port GPIOF
#define APPS2_3V3_Pin GPIO_PIN_10
#define APPS2_3V3_GPIO_Port GPIOF
#define APPS1_2V4_Pin GPIO_PIN_2
#define APPS1_2V4_GPIO_Port GPIOC
#define GPIO_PWM_1_Pin GPIO_PIN_0
#define GPIO_PWM_1_GPIO_Port GPIOA
#define GPIO_PWM_2_Pin GPIO_PIN_1
#define GPIO_PWM_2_GPIO_Port GPIOA
#define GPIO_PWM_3_Pin GPIO_PIN_2
#define GPIO_PWM_3_GPIO_Port GPIOA
#define BRAKE_LIGHT_3V3_Pin GPIO_PIN_13
#define BRAKE_LIGHT_3V3_GPIO_Port GPIOF
#define BUTTON_1_SIG_Pin GPIO_PIN_14
#define BUTTON_1_SIG_GPIO_Port GPIOF
#define BUTTON_2_SIG_Pin GPIO_PIN_15
#define BUTTON_2_SIG_GPIO_Port GPIOF
#define BUTTON_3_SIG_Pin GPIO_PIN_0
#define BUTTON_3_SIG_GPIO_Port GPIOG
#define BUTTON_4_SIG_Pin GPIO_PIN_1
#define BUTTON_4_SIG_GPIO_Port GPIOG
#define BMS_3V3_Pin GPIO_PIN_7
#define BMS_3V3_GPIO_Port GPIOE
#define IMD_3V3_Pin GPIO_PIN_8
#define IMD_3V3_GPIO_Port GPIOE
#define BSPD_3V3_Pin GPIO_PIN_9
#define BSPD_3V3_GPIO_Port GPIOE
#define GPIO_PWM_4_Pin GPIO_PIN_15
#define GPIO_PWM_4_GPIO_Port GPIOB
#define CAN1_L_Pin GPIO_PIN_0
#define CAN1_L_GPIO_Port GPIOD
#define CAN1_H_Pin GPIO_PIN_1
#define CAN1_H_GPIO_Port GPIOD
#define CAN2_L_Pin GPIO_PIN_5
#define CAN2_L_GPIO_Port GPIOB
#define CAN2_H_Pin GPIO_PIN_6
#define CAN2_H_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
