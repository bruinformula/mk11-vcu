/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "fdcan.h"
#include "gpio.h"
#include "i2s.h"
#include "tim.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "motor_control.h"
#include "prchg.h"
#include "startup_sound.h"
#include <stdbool.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

// rtd shits
// active rtd noise is the OG one in startup_sound.h
// shit is jank as fuck
#define WAV_HEADER_SIZE 44
#define WAV_DATA_SIZE (startup_sound_len - WAV_HEADER_SIZE)
#define TOTAL_HALFWORDS (WAV_DATA_SIZE / 2)
#define CHUNK_SIZE_HALFWORDS 4096U
#define TEST_TONE_FRAME_COUNT 4410U
#define TEST_TONE_CHANNEL_COUNT 2U
#define TEST_TONE_HALFWORD_COUNT                                             \
  (TEST_TONE_FRAME_COUNT * TEST_TONE_CHANNEL_COUNT)
#define TEST_TONE_PERIOD_FRAMES 44U
#define BOOT_AUDIO_SELFTEST 0U
#define DIRECT_RTD_AUDIO_BUTTON 1U
#define AUDIO_RETRIGGER_GUARD_MS 750U
#define RTD_VOLUME_SHIFT 2U
#define TEST_TONE_HIGH_SAMPLE ((uint16_t)0x1000)
#define TEST_TONE_LOW_SAMPLE ((uint16_t)0xF000)


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// uint16_t ADC_VAL[3];
// float voltage_values[3];

int fdcan1_debug_cb = 0;
int fdcan2_debug_cb = 0;

volatile uint32_t fdcan_rx_count = 0; /* Counter for received messages */
volatile uint32_t fdcan_tx_count = 0; /* Counter for transmitted messages */
volatile uint32_t fdcan_rx_error_count = 0; /* RX error counter */

volatile bool bms_fault;
volatile bool imd_fault;
volatile bool bspd_fault;

volatile bool rtd_button_pressed;
volatile bool prchg_button_pressed;

volatile uint8_t rtd_debug;
volatile uint8_t prchg_debug;

bool ready_to_drive = false;
DebugStats_t debug = {0};

static uint32_t wavPos = 0;
static const uint16_t *wavePCM = NULL;
static uint32_t halfwordCount = 0;
static uint8_t waveFinished = 0;
static uint16_t testToneBuffer[TEST_TONE_HALFWORD_COUNT];
static uint16_t scaledPlaybackBuffer[CHUNK_SIZE_HALFWORDS];
static bool testToneInitialized = false;
static bool setReadyToDriveOnPlaybackComplete = false;
static uint32_t lastPlaybackStartTick = 0;
static bool hasStartedPlayback = false;
static bool scalePlaybackSamples = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void fillTestToneBuffer(void) {
  if (testToneInitialized) {
    return;
  }

  for (uint32_t frame = 0; frame < TEST_TONE_FRAME_COUNT; frame++) {
    uint16_t sample =
        ((frame % TEST_TONE_PERIOD_FRAMES) < (TEST_TONE_PERIOD_FRAMES / 2U))
            ? TEST_TONE_HIGH_SAMPLE
            : TEST_TONE_LOW_SAMPLE;
    uint32_t offset = frame * TEST_TONE_CHANNEL_COUNT;

    testToneBuffer[offset] = sample;
    testToneBuffer[offset + 1U] = sample;
  }

  testToneInitialized = true;
}

static void captureI2SDebugState(void) {
  debug.i2s_state = HAL_I2S_GetState(&hi2s2);
  debug.i2s_error = HAL_I2S_GetError(&hi2s2);
  debug.spi123_clock_source = __HAL_RCC_GET_SPI123_SOURCE();
  debug.spi123_clock_hz = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SPI123);
  debug.spi2_cr1 = hi2s2.Instance->CR1;
  debug.spi2_i2scfgr = hi2s2.Instance->I2SCFGR;
}

static bool audioPlaybackAllowed(void) {
  uint32_t now = HAL_GetTick();
  if (hasStartedPlayback &&
      (now - lastPlaybackStartTick) < AUDIO_RETRIGGER_GUARD_MS) {
    return false;
  }

  lastPlaybackStartTick = now;
  hasStartedPlayback = true;
  return true;
}

static uint16_t *preparePlaybackChunk(const uint16_t *source, uint16_t count) {
  if (!scalePlaybackSamples) {
    return (uint16_t *)source;
  }

  for (uint16_t i = 0; i < count; i++) {
    int16_t sample = (int16_t)source[i];
    scaledPlaybackBuffer[i] = (uint16_t)(sample >> RTD_VOLUME_SHIFT);
  }

  return scaledPlaybackBuffer;
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
  debug.i2s_tx_hit++;
  if (hi2s->Instance == SPI2 && !waveFinished) {
    // finished one chunk
    if (wavPos < halfwordCount) {
      // Start the next chunk
      uint32_t remain = halfwordCount - wavPos;
      uint16_t thisChunk = (remain > CHUNK_SIZE_HALFWORDS)
                               ? CHUNK_SIZE_HALFWORDS
                               : (uint16_t)remain;
      const uint16_t *chunkPtr = wavePCM + wavPos;
      wavPos += thisChunk;
      debug.wave_halfword_position = wavPos;

      HAL_I2S_Transmit_DMA(&hi2s2, preparePlaybackChunk(chunkPtr, thisChunk),
                           thisChunk);
    } else {
      // entire wave is done
      waveFinished = 1;
      lastPlaybackStartTick = HAL_GetTick();
      debug.playback_finished++;
      debug.wave_halfword_position = halfwordCount;
      if (setReadyToDriveOnPlaybackComplete) {
        ready_to_drive = true;
        setReadyToDriveOnPlaybackComplete = false;
      }
    }
  }
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s) {
  if (hi2s->Instance == SPI2) {
    debug.i2s_error_cb_hit++;
    captureI2SDebugState();
  }
}

void playReadyToDriveSound() {
  if (HAL_I2S_GetState(&hi2s2) != HAL_I2S_STATE_READY ||
      !audioPlaybackAllowed()) {
    return; // Avoid re-triggering if busy
  }

  setReadyToDriveOnPlaybackComplete = true;
  scalePlaybackSamples = true;
  debug.playback_kind = 2;
  debug.playback_started++;
  wavePCM = (const uint16_t *)(&startup_sound[WAV_HEADER_SIZE]);
  halfwordCount = TOTAL_HALFWORDS;
  wavPos = 0;
  waveFinished = 0;
  debug.wave_halfword_count = halfwordCount;
  debug.wave_halfword_position = 0;

  while (wavPos < halfwordCount) {
    uint32_t remain = halfwordCount - wavPos;
    uint16_t thisChunk =
        (remain > CHUNK_SIZE_HALFWORDS) ? CHUNK_SIZE_HALFWORDS
                                        : (uint16_t)remain;
    const uint16_t *chunkPtr = wavePCM + wavPos;

    debug.last_i2s_status =
        HAL_I2S_Transmit(&hi2s2, preparePlaybackChunk(chunkPtr, thisChunk),
                         thisChunk, 1000);
    captureI2SDebugState();
    if (debug.last_i2s_status != HAL_OK) {
      return;
    }

    wavPos += thisChunk;
    debug.wave_halfword_position = wavPos;
  }

  waveFinished = 1;
  lastPlaybackStartTick = HAL_GetTick();
  debug.playback_finished++;
  debug.wave_halfword_position = halfwordCount;
  ready_to_drive = true;
  setReadyToDriveOnPlaybackComplete = false;
}

void testSpeaker() {
  if (!audioPlaybackAllowed()) {
    return;
  }

  if (HAL_I2S_GetState(&hi2s2) != HAL_I2S_STATE_READY) {
    captureI2SDebugState();
    if (debug.i2s_state == HAL_I2S_STATE_BUSY ||
        debug.i2s_state == HAL_I2S_STATE_BUSY_TX ||
        debug.i2s_state == HAL_I2S_STATE_BUSY_TX_RX) {
      debug.last_dma_stop_status = HAL_I2S_DMAStop(&hi2s2);
    }

    captureI2SDebugState();
    if (debug.i2s_state != HAL_I2S_STATE_READY) {
      return;
    }
  }

  setReadyToDriveOnPlaybackComplete = false;
  scalePlaybackSamples = false;
  debug.playback_kind = 1;
  debug.playback_started++;
  wavePCM = testToneBuffer;
  halfwordCount = TEST_TONE_HALFWORD_COUNT;
  wavPos = 0;
  waveFinished = 0;
  debug.wave_halfword_count = halfwordCount;
  debug.wave_halfword_position = 0;

  uint32_t remain = halfwordCount - wavPos;
  uint16_t thisChunk =
      (remain > CHUNK_SIZE_HALFWORDS) ? CHUNK_SIZE_HALFWORDS : (uint16_t)remain;
  const uint16_t *chunkPtr = wavePCM + wavPos;
  wavPos += thisChunk;
  debug.wave_halfword_position = wavPos;

  debug.last_i2s_status =
      HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t *)chunkPtr, thisChunk);
  captureI2SDebugState();
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  // Conversions for debugging purposes; probe the inputs on VCU PCB and compare
  // to live expressions
  voltage_values[0] =
      (ADC_VAL[0] / 4095.0) * 3.3; // CHANNEL 0: APPS1 (0 - 2.4V)
  voltage_values[1] =
      (ADC_VAL[1] / 4095.0) * 3.3; // CHANNEL 6: APPS2 (0 - 3.3V)
  voltage_values[2] = (ADC_VAL[2] / 4095.0) * 3.3; // CHANNEL 7: BSE (0 - 3.3V)

  if (ready_to_drive == true && inverter_precharged == true) {
    // DRIVE MODE!

    pedal_percents[0] = ((float)ADC_VAL[0] - APPS1_ADC_MIN_VAL) /
                        (APPS1_ADC_MAX_VAL - APPS1_ADC_MIN_VAL);
    pedal_percents[1] = ((float)ADC_VAL[1] - APPS2_ADC_MIN_VAL) /
                        (APPS2_ADC_MAX_VAL - APPS2_ADC_MIN_VAL);
    pedal_percents[2] = ((float)ADC_VAL[2] - BSE_ADC_MIN_VAL) /
                        (BSE_ADC_MAX_VAL - BSE_ADC_MIN_VAL);

    calculateTorqueRequest();
    checkAPPS_Plausibility();
    checkBSE_Plausibility();
    checkAPPS_BSE_Crosscheck();
    sendTorqueRequest((int)(requestedTorque * 10));
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  // Change BMS Fault Logic? --> NOT Latching Fault, can re-enter Precharge if
  // BMS Fault Clears!
  if (GPIO_Pin == BMS_FAULT_Pin) {
    bms_fault = true;
    stopMotor();
    Error_Handler();
  }

  if (GPIO_Pin == IMD_FAULT_Pin) {
    imd_fault = true;
    stopMotor();
    Error_Handler();
  }

  if (GPIO_Pin == BSPD_FAULT_Pin) {
    bspd_fault = true;
    stopMotor();
    Error_Handler();
  }

  if (GPIO_Pin == RTD_BTN_Pin) {
    rtd_debug++;
    if (HAL_GPIO_ReadPin(RTD_BTN_GPIO_Port, RTD_BTN_Pin) == GPIO_PIN_SET) {
#if DIRECT_RTD_AUDIO_BUTTON
      debug.trigger_rtd_sound = 1;
      playReadyToDriveSound();
#else
      rtd_button_pressed = true;
      __HAL_TIM_SET_COUNTER(&htim1, 0);
      HAL_TIM_Base_Start_IT(&htim1);
#endif
    } else {
      // Button Released!
      rtd_button_pressed = false;
      HAL_TIM_Base_Stop_IT(&htim1);
    }
  }

  if (GPIO_Pin == PRCHG_BTN_Pin) {
    prchg_debug++;
    if (HAL_GPIO_ReadPin(PRCHG_BTN_GPIO_Port, PRCHG_BTN_Pin) == GPIO_PIN_SET) {
      prchg_button_pressed = true;
      __HAL_TIM_SET_COUNTER(&htim1, 0);
      HAL_TIM_Base_Start_IT(&htim1);
    } else {
      // Button Released!
      prchg_button_pressed = false;
      HAL_TIM_Base_Stop_IT(&htim1);
    }
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM1) {

    if (prchg_button_pressed == true) {
      configurePrechargeMessage();
      sendPrechargeRequest();

      // COMMENT OUT WHEN CAN IS BEING USED... THIS IS FOR DEBUGGING/TESTING TO
      // "SKIP" ACTUAL PRECHARGE inverter_precharged = true;
    }

    if (rtd_button_pressed == true) {
#if DIRECT_RTD_AUDIO_BUTTON
      HAL_TIM_Base_Stop_IT(&htim1);
      rtd_button_pressed = false;
      return;
#else
      if (inverter_precharged == false) {
        return;
      }

      if (ADC_VAL[2] > BSE_ACTIVATED_ADC_THRESHOLD) {
        debug.trigger_rtd_sound = 1; // Trigger from main loop instead of ISR
      } else {
        ready_to_drive = false;
      }
#endif
    }
  }
}

// FDCAN1 Callback
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan,
                               uint32_t RxFifo0ITs) {
  fdcan1_debug_cb++;
  if (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) {

    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader1, RxData1) !=
        HAL_OK) {
      fdcan_rx_error_count++;
      return;
    }

    fdcan_rx_count++;
    FDCAN1_Rx_Handler();
  }
}

// FDCAN2 Callback
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan,
                               uint32_t RxFifo1ITs) {
  fdcan2_debug_cb++;
  if (RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) {

    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &RxHeader2, RxData2) !=
        HAL_OK) {
      fdcan_rx_error_count++;
      return;
    }

    fdcan_rx_count++;
    FDCAN2_Rx_Handler();
  }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  /* USER CODE BEGIN 1 */
  debug.execution_marker = 1;
  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  HAL_StatusTypeDef last_i2s_status_local = MX_I2S2_Init();
  debug.last_i2s_status = last_i2s_status_local;
  captureI2SDebugState();
  fillTestToneBuffer();
#if BOOT_AUDIO_SELFTEST
  debug.execution_marker = 13;
  HAL_Delay(50);
  testSpeaker();
  debug.execution_marker = 14;
#endif
  MX_ADC3_Init();
  MX_FDCAN1_Init();
  MX_FDCAN2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  // FDCAN1 FILTER SETUP
  FDCAN_FilterTypeDef sFilterConfig_1 = {0};
  sFilterConfig_1.IdType = FDCAN_STANDARD_ID;
  sFilterConfig_1.FilterIndex = 0;
  sFilterConfig_1.FilterType = FDCAN_FILTER_RANGE;
  sFilterConfig_1.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  sFilterConfig_1.FilterID1 = 0x000;
  sFilterConfig_1.FilterID2 = 0x7FF;
  sFilterConfig_1.RxBufferIndex = 0;
  if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig_1) != HAL_OK) {
    /* Filter configuration Error */
    Error_Handler();
  }

  // FDCAN2 FILTER SETUP
  FDCAN_FilterTypeDef sFilterConfig_2 = {0};
  sFilterConfig_2.IdType = FDCAN_STANDARD_ID;
  sFilterConfig_2.FilterIndex = 0;
  sFilterConfig_2.FilterType = FDCAN_FILTER_RANGE;
  sFilterConfig_2.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
  sFilterConfig_2.FilterID1 = 0x000;
  sFilterConfig_2.FilterID2 = 0x7FF;
  sFilterConfig_2.RxBufferIndex = 0;
  if (HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilterConfig_2) != HAL_OK) {
    /* Filter configuration Error */
    Error_Handler();
  }

  // Start FDCAN1
  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
    Error_Handler();
  }

  // Start FDCAN2
  if (HAL_FDCAN_Start(&hfdcan2) != HAL_OK) {
    Error_Handler();
  }

  // Activate the notification for new data in FIFO0 for FDCAN1
  if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE,
                                     0) != HAL_OK) {
    /* Notification Error */
    Error_Handler();
  }

  // Activate the notification for new data in FIFO1 for FDCAN2
  if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO1_NEW_MESSAGE,
                                     0) != HAL_OK) {
    /* Notification Error */
    Error_Handler();
  }

  debug.execution_marker = 2;
  HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
  HAL_ADC_Start_DMA(&hadc3, (uint32_t *)ADC_VAL, 3);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    debug.execution_marker = 3;
    debug.loop_counter++;
    if (debug.trigger_test_sound) {
      debug.execution_marker = 4;
      debug.trigger_test_sound = 0;
      testSpeaker();
    }

    if (debug.trigger_rtd_sound) {
      debug.trigger_rtd_sound = 0;
      playReadyToDriveSound();
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
   */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
   */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
  }

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 12;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void) {
  /* Disables the MPU */
  HAL_MPU_Disable();
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  debug.execution_marker = 88;
  __disable_irq();
  HAL_I2S_DMAStop(&hi2s2);
  HAL_ADC_Stop_DMA(&hadc3);

  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
