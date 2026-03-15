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
#include <stdbool.h>
#include <stdint.h>


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef struct {
  volatile uint32_t
      execution_marker; // 0=start, 1=init, 2=init_done, 3=loop, 4=playing
  volatile uint32_t loop_counter;
  volatile uint32_t dma_irq_hit;
  volatile uint32_t i2s_tx_hit;
  volatile uint32_t i2s_error_cb_hit;
  volatile uint32_t i2s_state;
  volatile uint32_t i2s_error;
  volatile uint32_t last_i2s_status;
  volatile uint32_t spi123_clock_source;
  volatile uint32_t spi123_clock_hz;
  volatile uint32_t spi2_cr1;
  volatile uint32_t spi2_i2scfgr;
  volatile uint32_t last_dma_stop_status;
  volatile uint32_t playback_kind;
  volatile uint32_t playback_started;
  volatile uint32_t playback_finished;
  volatile uint32_t wave_halfword_count;
  volatile uint32_t wave_halfword_position;
  volatile uint32_t trigger_test_sound;
  volatile uint32_t trigger_rtd_sound;
} DebugStats_t;
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
extern DebugStats_t debug;
extern bool ready_to_drive;
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PRCHG_BTN_Pin GPIO_PIN_14
#define PRCHG_BTN_GPIO_Port GPIOF
#define PRCHG_BTN_EXTI_IRQn EXTI15_10_IRQn
#define RTD_BTN_Pin GPIO_PIN_15
#define RTD_BTN_GPIO_Port GPIOF
#define RTD_BTN_EXTI_IRQn EXTI15_10_IRQn
#define BMS_FAULT_Pin GPIO_PIN_7
#define BMS_FAULT_GPIO_Port GPIOE
#define BMS_FAULT_EXTI_IRQn EXTI9_5_IRQn
#define IMD_FAULT_Pin GPIO_PIN_8
#define IMD_FAULT_GPIO_Port GPIOE
#define IMD_FAULT_EXTI_IRQn EXTI9_5_IRQn
#define BSPD_FAULT_Pin GPIO_PIN_9
#define BSPD_FAULT_GPIO_Port GPIOE
#define BSPD_FAULT_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
