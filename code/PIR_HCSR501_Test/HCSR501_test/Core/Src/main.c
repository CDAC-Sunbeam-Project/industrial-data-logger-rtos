/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include<stdio.h>
#include<string.h>


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PIR_PIN					GPIO_PIN_2
#define PIR_PORT				GPIOC
#define BUZZER_PIN				GPIO_PIN_0
#define BUZZER_PORT				GPIOB
#define PIR_WARMUP_MS			30000U
#define PIR_DEBOUNCE_MS			50U
#define PIR_RECHECK_MS			120000U
#define POLL_INTERVAL_MS		50U
#define ALERT_BEEP_DURATION		100U
#define ALERT_BEEP_GAP			100U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */


static uint8_t pir_curr 			= 0;
static uint8_t pir_prev				= 0;
static uint8_t zone_occupied		= 0;
static uint32_t zone_entry_time		= 0;
static uint32_t last_recheck_time 	= 0;
static uint32_t total_events 		= 0;
static uint32_t system_start_time 	= 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int _write(int file, char *ptr, int len)
{
	HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, 100);
	return len;
}

void Buzzer_Beep(uint8_t count, uint32_t duration_ms)
{
	for (uint8_t i = 0; i < count; i++)
	{
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
		HAL_Delay(duration_ms);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
		if (i < count - 1) HAL_Delay(ALERT_BEEP_GAP);
	}

}

void print_uptime(void)
{
	uint32_t ms 		= HAL_GetTick() - system_start_time;
	uint32_t seconds	= (ms / 1000) % 60;
	uint32_t minutes	= (ms / 60000) % 60;
	uint32_t hours		= (ms / 3600000);
	printf("%02lu:%02lu:%02lu", hours, minutes, seconds);
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  system_start_time = HAL_GetTick();
  printf("\r\n==================================================\r\n");
  printf("	SERVER ROOM SECURITY MONITOR\r\n");
  printf("	STM32F407 Discovery - PIR v1.0\r\n");
  printf("\r\n==============================================\r\n");

  printf("[BOOT] Buzzer self-test...\r\n");
  Buzzer_Beep(2, 50);
  printf("[BOOT] Buzzer OK\r\n\n");

  printf("[PIR]	Warming up - stand clear\r\n");
  printf("[PIR]	Please wait %lu seconds...\r\n\n", PIR_WARMUP_MS / 1000);
  HAL_Delay(PIR_WARMUP_MS);

  printf("[PIR]	Sensor ready - monitoring PC2\r\n");
  printf("[SYSTEM]	Monitoring ACTIVE\r\n\n");


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      pir_curr = HAL_GPIO_ReadPin(PIR_PORT, PIR_PIN);
      uint32_t now = HAL_GetTick();

      /* RISING EDGE — entry */
      if (pir_curr == GPIO_PIN_SET && pir_prev == GPIO_PIN_RESET)
      {
          HAL_Delay(PIR_DEBOUNCE_MS);
          if (HAL_GPIO_ReadPin(PIR_PORT, PIR_PIN) == GPIO_PIN_SET)
          {
              if (!zone_occupied)
              {
                  zone_occupied     = 1;
                  zone_entry_time   = HAL_GetTick();
                  last_recheck_time = zone_entry_time;
                  total_events++;

                  printf("=============================================\r\n");
                  printf("  [!] MOTION DETECTED - ZONE ENTERED\r\n");
                  printf("=============================================\r\n");
                  printf("  Event  : #%lu\r\n", total_events);
                  printf("  Uptime : ");
                  print_uptime();
                  printf("\r\n");
                  printf("  Status : OCCUPIED\r\n");
                  printf("=============================================\r\n\n");

                  Buzzer_Beep(1, ALERT_BEEP_DURATION);
              }
          }
          else
          {
              printf("[PIR] Noise filtered\r\n");
          }
      }

      /* STILL OCCUPIED — re-alert every 2 min */
      if (zone_occupied && pir_curr == GPIO_PIN_SET)
      {
          if ((now - last_recheck_time) >= PIR_RECHECK_MS)
          {
              last_recheck_time = now;
              uint32_t dur = now - zone_entry_time;
              printf("[!] STILL OCCUPIED - %lu min %lu sec\r\n",
                     dur / 60000, (dur % 60000) / 1000);
              Buzzer_Beep(2, ALERT_BEEP_DURATION);
          }
      }

      /* FALLING EDGE — exit */
      if (pir_curr == GPIO_PIN_RESET && pir_prev == GPIO_PIN_SET)
      {
          if (zone_occupied)
          {
              zone_occupied = 0;
              uint32_t dur  = now - zone_entry_time;
              printf("=============================================\r\n");
              printf("  [OK] ZONE CLEARED\r\n");
              printf("=============================================\r\n");
              printf("  Duration : %lu min %lu sec\r\n",
                     dur / 60000, (dur % 60000) / 1000);
              printf("  Total    : %lu events\r\n", total_events);
              printf("  Status   : SECURE\r\n");
              printf("=============================================\r\n\n");
          }
      }

      pir_prev = pir_curr;
      HAL_Delay(POLL_INTERVAL_MS);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PIR_IN_Pin */
  GPIO_InitStruct.Pin = PIR_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(PIR_IN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Buzzer_Pin */
  GPIO_InitStruct.Pin = Buzzer_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Buzzer_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
