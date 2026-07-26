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
#define SENSOR_ADDR (0x76 << 1)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

static uint16_t dig_T1, dig_P1;
static int16_t  dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1, dig_H3;
static int16_t  dig_H2, dig_H4, dig_H5;
static int8_t   dig_H6;
static int32_t  t_fine;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void BME280_ReadCalibration(void)
{
    uint8_t c1[26];
    HAL_I2C_Mem_Read(&hi2c1, SENSOR_ADDR, 0x88, I2C_MEMADD_SIZE_8BIT, c1, 26, 100);

    dig_T1 = (uint16_t)(c1[1] << 8 | c1[0]);
    dig_T2 = (int16_t)(c1[3] << 8 | c1[2]);
    dig_T3 = (int16_t)(c1[5] << 8 | c1[4]);
    dig_P1 = (uint16_t)(c1[7] << 8 | c1[6]);
    dig_P2 = (int16_t)(c1[9] << 8 | c1[8]);
    dig_P3 = (int16_t)(c1[11] << 8 | c1[10]);
    dig_P4 = (int16_t)(c1[13] << 8 | c1[12]);
    dig_P5 = (int16_t)(c1[15] << 8 | c1[14]);
    dig_P6 = (int16_t)(c1[17] << 8 | c1[16]);
    dig_P7 = (int16_t)(c1[19] << 8 | c1[18]);
    dig_P8 = (int16_t)(c1[21] << 8 | c1[20]);
    dig_P9 = (int16_t)(c1[23] << 8 | c1[22]);
    dig_H1 = c1[25];

    uint8_t c2[7];
    HAL_I2C_Mem_Read(&hi2c1, SENSOR_ADDR, 0xE1, I2C_MEMADD_SIZE_8BIT, c2, 7, 100);

    dig_H2 = (int16_t)(c2[1] << 8 | c2[0]);
    dig_H3 = c2[2];
    dig_H4 = (int16_t)(((int8_t)c2[3] << 4) | (c2[4] & 0x0F));
    dig_H5 = (int16_t)(((int8_t)c2[5] << 4) | (c2[4] >> 4));
    dig_H6 = (int8_t)c2[6];
}

void BME280_Init(void)
{
    uint8_t ctrl_hum  = 0x01;   // humidity oversampling x1
    uint8_t config_reg = 0x00;  // filter off, standby time minimal
    uint8_t ctrl_meas = 0x27;   // temp x1, pressure x1, normal mode

    HAL_I2C_Mem_Write(&hi2c1, SENSOR_ADDR, 0xF2, I2C_MEMADD_SIZE_8BIT, &ctrl_hum, 1, 100);
    HAL_I2C_Mem_Write(&hi2c1, SENSOR_ADDR, 0xF5, I2C_MEMADD_SIZE_8BIT, &config_reg, 1, 100);
    HAL_I2C_Mem_Write(&hi2c1, SENSOR_ADDR, 0xF4, I2C_MEMADD_SIZE_8BIT, &ctrl_meas, 1, 100);

    HAL_Delay(100); // let the first measurement complete
}

float BME280_CompensateTemperature(int32_t adc_T)
{
    float var1, var2, T;
    var1 = ((float)adc_T / 16384.0f) - ((float)dig_T1 / 1024.0f);
    var2 = var1 * (float)dig_T2;
    var1 = (((float)adc_T / 131072.0f) - ((float)dig_T1 / 8192.0f)) *
           (((float)adc_T / 131072.0f) - ((float)dig_T1 / 8192.0f));
    var1 = var1 * (float)dig_T3;
    t_fine = (int32_t)(var2 + var1);
    T = (var2 + var1) / 5120.0f;
    return T; // degrees Celsius
}

float BME280_CompensatePressure(int32_t adc_P)
{
    float var1, var2, P;
    var1 = ((float)t_fine / 2.0f) - 64000.0f;
    var2 = var1 * var1 * (float)dig_P6 / 32768.0f;
    var2 = var2 + var1 * (float)dig_P5 * 2.0f;
    var2 = (var2 / 4.0f) + ((float)dig_P4 * 65536.0f);
    var1 = ((float)dig_P3 * var1 * var1 / 524288.0f + (float)dig_P2 * var1) / 524288.0f;
    var1 = (1.0f + var1 / 32768.0f) * (float)dig_P1;
    if (var1 == 0.0f) return 0;
    P = 1048576.0f - (float)adc_P;
    P = (P - (var2 / 4096.0f)) * 6250.0f / var1;
    var1 = (float)dig_P9 * P * P / 2147483648.0f;
    var2 = P * (float)dig_P8 / 32768.0f;
    P = P + (var1 + var2 + (float)dig_P7) / 16.0f;
    return P / 100.0f; // hPa
}

float BME280_CompensateHumidity(int32_t adc_H)
{
    float var_H;
    var_H = ((float)t_fine - 76800.0f);
    var_H = (adc_H - ((float)dig_H4 * 64.0f + ((float)dig_H5 / 16384.0f) * var_H)) *
            ((float)dig_H2 / 65536.0f * (1.0f + (float)dig_H6 / 67108864.0f * var_H *
            (1.0f + (float)dig_H3 / 67108864.0f * var_H)));
    var_H = var_H * (1.0f - (float)dig_H1 * var_H / 524288.0f);
    if (var_H > 100.0f) var_H = 100.0f;
    else if (var_H < 0.0f) var_H = 0.0f;
    return var_H; // %RH
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
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  BME280_ReadCalibration();
  BME280_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  uint8_t raw[8];
	  HAL_I2C_Mem_Read(&hi2c1, SENSOR_ADDR, 0xF7, I2C_MEMADD_SIZE_8BIT, raw, 8, 100);

	  int32_t adc_P = (int32_t)((raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4));
	  int32_t adc_T = (int32_t)((raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4));
	  int32_t adc_H = (int32_t)((raw[6] << 8)  |  raw[7]);

	  float temperature = BME280_CompensateTemperature(adc_T);
	  float pressure    = BME280_CompensatePressure(adc_P);
	  float humidity     = BME280_CompensateHumidity(adc_H);

	  char msg[100];
	  sprintf(msg, "Temp: %.2f C | Pressure: %.2f hPa | Humidity: %.2f %%\r\n",temperature, pressure, humidity);
	  HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);

	  HAL_Delay(1000);
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

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
