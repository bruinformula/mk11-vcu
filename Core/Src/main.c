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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t ADC_VAL[3];
float voltage_values[3];
int indx = 0;
int debug1_cb = 0;
int debug2_cb = 0;

FDCAN_TxHeaderTypeDef   TxHeader1;
FDCAN_RxHeaderTypeDef   RxHeader1;
uint8_t               TxData1[8] = {66, 77, 66, 77, 66, 77, 66, 77};  /* VCU TX pattern */
uint8_t               RxData1[8] = {66, 99, 66, 99, 66, 99, 66, 99};  /* VCU RX pattern */

FDCAN_TxHeaderTypeDef   TxHeader2;
FDCAN_RxHeaderTypeDef   RxHeader2;
uint8_t               TxData2[8] = {66, 77, 66, 77, 66, 77, 66, 77};  /* VCU TX pattern */
uint8_t               RxData2[8] = {66, 99, 66, 99, 66, 99, 66, 99};  /* VCU RX pattern */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/** TEST MODE SELECTION **/
/* Set to 1 for VCU transmit mode, 0 for VCU receive mode */
#define VCU_TRANSMIT_MODE 0

/* Set to 1 for FDCAN1, 2 for FDCAN2 */
#define VCU_FDCAN_SELECT 2

/* Set to 1 to echo received messages back (for normal mode testing) */
/* Set to 0 to disable echo (for external loopback testing) */
#define VCU_ECHO_ENABLE 1

/** FDCAN Test Debug Variables (matches BMS naming) **/
volatile uint32_t fdcan_rx_count = 0;          /* Counter for received messages */
volatile uint32_t fdcan_last_rx_id = 0;        /* Last received message ID */
volatile uint8_t fdcan_last_rx_data[8] = {0};  /* Last received data */
volatile uint32_t fdcan_rx_error_count = 0;    /* RX error counter */
volatile uint32_t fdcan_tx_count = 0;          /* Counter for transmitted messages */

/** FDCAN Status Registers **/
volatile uint32_t fdcan_psr_register = 0;      /* Protocol Status Register */
volatile uint32_t fdcan_cccr_register = 0;     /* Control and Configuration Register */
volatile uint32_t fdcan_ecr_register = 0;      /* Error Counter Register */
volatile uint32_t fdcan_txfqs_register = 0;    /* TX FIFO/Queue Status Register */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	voltage_values[0] = (ADC_VAL[0]/4095.0)*3.3; // CHANNEL 0: APPS1 (0 - 2.4V)
	voltage_values[1] = (ADC_VAL[1]/4095.0)*3.3; // CHANNEL 6: APPS2 (0 - 3.3V)
	voltage_values[2] = (ADC_VAL[2]/4095.0)*3.3; // CHANNEL 7: BSE (0 - 3.3V)
}

// FDCAN1 Callback (matches BMS logic)
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
	debug1_cb++;
	if (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)
	{
		FDCAN_RxHeaderTypeDef localRxHeader;
		uint8_t localRxData[8];

		if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &localRxHeader, localRxData) != HAL_OK)
		{
			fdcan_rx_error_count++;
			return;
		}

		/* Store received data for debugging */
		fdcan_rx_count++;
		fdcan_last_rx_id = localRxHeader.Identifier;
		for (int i = 0; i < 8; i++) {
			fdcan_last_rx_data[i] = localRxData[i];
		}

		/* Also copy to global RxData/RxHeader for debugger visibility */
		RxHeader1 = localRxHeader;
		for (int i = 0; i < 8; i++) {
			RxData1[i] = localRxData[i];
		}

#if VCU_ECHO_ENABLE
		/* Echo received message back on FDCAN1 */
		if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader1, localRxData) == HAL_OK) {
			fdcan_tx_count++;
		}
#endif
	}
}

// FDCAN2 Callback (matches BMS logic)
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
	debug2_cb++;
	if (RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE)
	{
		FDCAN_RxHeaderTypeDef localRxHeader;
		uint8_t localRxData[8];

		if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &localRxHeader, localRxData) != HAL_OK)
		{
			fdcan_rx_error_count++;
			return;
		}

		/* Store received data for debugging */
		fdcan_rx_count++;
		fdcan_last_rx_id = localRxHeader.Identifier;
		for (int i = 0; i < 8; i++) {
			fdcan_last_rx_data[i] = localRxData[i];
		}

		/* Also copy to global RxData/RxHeader for debugger visibility */
		RxHeader2 = localRxHeader;
		for (int i = 0; i < 8; i++) {
			RxData2[i] = localRxData[i];
		}

#if VCU_ECHO_ENABLE
		/* Echo received message back on FDCAN2 */
		if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader2, localRxData) == HAL_OK) {
			fdcan_tx_count++;
		}
#endif
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
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
  MX_ADC3_Init();
  MX_FDCAN1_Init();
  MX_FDCAN2_Init();
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
  if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig_1) != HAL_OK)
  {
    /* Filter configuration Error */
    Error_Handler();
  }

  // Configure FDCAN1 Global Filter - Accept All to FIFO0
  if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
                                    FDCAN_ACCEPT_IN_RX_FIFO0,  // Accept non-matching standard IDs to FIFO0
                                    FDCAN_REJECT,              // Reject extended IDs (not used)
                                    DISABLE,
                                    DISABLE) != HAL_OK) {
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
  if (HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilterConfig_2) != HAL_OK)
  {
    /* Filter configuration Error */
    Error_Handler();
  }
  // Configure FDCAN2 Global Filter - Accept All
  if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan2,
                                    FDCAN_ACCEPT_IN_RX_FIFO1,  // Accept ALL standard IDs to FIFO1
                                    FDCAN_REJECT,              // Reject extended IDs (not used)
                                    DISABLE,
                                    DISABLE) != HAL_OK) {
    Error_Handler();
  }

  // Start FDCAN1
  if(HAL_FDCAN_Start(&hfdcan1)!= HAL_OK) {
	  Error_Handler();
  }

  // Start FDCAN2
  if(HAL_FDCAN_Start(&hfdcan2)!= HAL_OK) {
	  Error_Handler();
  }

  // Activate the notification for new data in FIFO0 for FDCAN1
  if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
	  /* Notification Error */
	  Error_Handler();
  }

  // Activate the notification for new data in FIFO1 for FDCAN2
  if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0) != HAL_OK)
  {
	  /* Notification Error */
	  Error_Handler();
  }

  // Configure TX Header for FDCAN1
  TxHeader1.Identifier = 0x67;  /* VCU TX ID */
  TxHeader1.IdType = FDCAN_STANDARD_ID;
  TxHeader1.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader1.DataLength = FDCAN_DLC_BYTES_8;
  TxHeader1.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader1.BitRateSwitch = FDCAN_BRS_OFF;
  TxHeader1.FDFormat = FDCAN_CLASSIC_CAN;
  TxHeader1.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
  TxHeader1.MessageMarker = 0;

  // Configure TX Header for FDCAN2
  TxHeader2.Identifier = 0x67;  /* VCU TX ID */
  TxHeader2.IdType = FDCAN_STANDARD_ID;
  TxHeader2.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader2.DataLength = FDCAN_DLC_BYTES_8;
  TxHeader2.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader2.BitRateSwitch = FDCAN_BRS_OFF;
  TxHeader2.FDFormat = FDCAN_CLASSIC_CAN;
  TxHeader2.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
  TxHeader2.MessageMarker = 0;

//
//  // Initialize RX Header for FDCAN1 (optional - will be overwritten by GetRxMessage)
//   RxHeader1.Identifier = 0x11;
//   RxHeader1.IdType = FDCAN_STANDARD_ID;
//   RxHeader1.RxFrameType = FDCAN_DATA_FRAME;
//   RxHeader1.DataLength = FDCAN_DLC_BYTES_8;
//   RxHeader1.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
//   RxHeader1.BitRateSwitch = FDCAN_BRS_OFF;
//   RxHeader1.FDFormat = FDCAN_CLASSIC_CAN;
//   RxHeader1.RxTimestamp = 0;
//   RxHeader1.FilterIndex = 0;
//   RxHeader1.IsFilterMatchingFrame = 0;
//
//   // Initialize RX Header for FDCAN2 (optional - will be overwritten by GetRxMessage)
//   RxHeader2.Identifier = 0x22;
//   RxHeader2.IdType = FDCAN_STANDARD_ID;
//   RxHeader2.RxFrameType = FDCAN_DATA_FRAME;
//   RxHeader2.DataLength = FDCAN_DLC_BYTES_8;
//   RxHeader2.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
//   RxHeader2.BitRateSwitch = FDCAN_BRS_OFF;
//   RxHeader2.FDFormat = FDCAN_CLASSIC_CAN;
//   RxHeader2.RxTimestamp = 0;
//   RxHeader2.FilterIndex = 0;
//   RxHeader2.IsFilterMatchingFrame = 0;

//  HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
//  HAL_ADC_Start_DMA(&hadc3, (uint32_t*) ADC_VAL, 3);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /* ========== FDCAN TEST MAIN LOOP ========== */
  while (1)
  {
#if VCU_TRANSMIT_MODE
	  /* ========== VCU TRANSMIT MODE ========== */
	  /* VCU transmits ID 0x676 to BMS every 1 second */
	  /* BMS should receive and log the message */
  #if (VCU_FDCAN_SELECT == 1)
	  if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader1, TxData1) == HAL_OK) {
		  fdcan_tx_count++;  /* Increment on successful TX */
	  }
  #else
	  if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader2, TxData2) == HAL_OK) {
		  fdcan_tx_count++;  /* Increment on successful TX */
	  }
  #endif
	  HAL_Delay(1000);
#else
	  /* ========== VCU RECEIVE MODE ========== */
	  /* VCU receives messages from BMS (ID 0x696) */
	  /* Check fdcan_rx_count, fdcan_last_rx_data[], fdcan_last_rx_id in debugger */
	  HAL_Delay(1000);
#endif

	  /* Read status registers for debugging (common to both modes) */
#if (VCU_FDCAN_SELECT == 1)
	  fdcan_psr_register = hfdcan1.Instance->PSR;     /* Protocol Status */
	  fdcan_ecr_register = hfdcan1.Instance->ECR;     /* Error Counters */
	  fdcan_cccr_register = hfdcan1.Instance->CCCR;   /* Control Register */
	  fdcan_txfqs_register = hfdcan1.Instance->TXFQS; /* TX FIFO Queue Status */
#else
	  fdcan_psr_register = hfdcan2.Instance->PSR;     /* Protocol Status */
	  fdcan_ecr_register = hfdcan2.Instance->ECR;     /* Error Counters */
	  fdcan_cccr_register = hfdcan2.Instance->CCCR;   /* Control Register */
	  fdcan_txfqs_register = hfdcan2.Instance->TXFQS; /* TX FIFO Queue Status */
#endif
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
