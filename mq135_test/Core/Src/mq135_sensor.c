/*
 * mq135_sensor\.c
 *
 *  Created on: 27-Jul-2026
 *      Author: sunbeam
 */
#include "mq135_sensor.h"
#include <stdio.h>
#include"main.h"
#include <string.h>
#include <math.h>  // ← Added for powf() function

/* External references - Your original code uses these */
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;

/* Private variables */
static uint32_t adcValue = 0;
static float voltage = 0.0f;
static float ppm = 0.0f;  // ← Added for PPM

static void UART_Send(const char* str)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)str, strlen(str), 1000);
}

//static void Buzzer_On(void)
//{
//    HAL_GPIO_WritePin(GPIOD, BUZZER_PIN_13, GPIO_PIN_SET);
//}
//
//static void Buzzer_Off(void)
//{
//    HAL_GPIO_WritePin(GPIOD, BUZZER_PIN_13, GPIO_PIN_RESET);
//}

/* ============================================================
  FUNCTION ADDED -
   ============================================================ */

	static void MQ135_PrintStatus(uint32_t adcValue, float voltage, float ppm, uint8_t level)
{
    char msg[800];
    const char *levelText[] = {"🟢 LOW", "🟠 MEDIUM", "🔴 HIGH"};
    		 sprintf(msg,"\r\n╔═══════════════════════════════════════════════╗\r\n"
             	 	 	 	 "║     MQ135 Gas Sensor Report                   ║\r\n"
             	 	 	 	 "╠═══════════════════════════════════════════════╣\r\n"
    						 "║  ADC Value  : %4lu                            ║\r\n"
    						 "║  Voltage    : %.2f V                       	  ║\r\n"
                             "║  PPM        : %.1f                            ║\r\n"
                             "║  Status     : %s                			  ║\r\n"
                             "╚═══════════════════════════════════════════════╝\r\n",
							 (unsigned long)adcValue, voltage, ppm, levelText[level]);

    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

/**
 * @brief MQ135 FreeRTOS Task - Your exact logic preserved
 */
void MQ135_Task(void *argument)
{
    (void)argument;

    /* ---------------------------------------------------------------
     * STEP 0: Show a one-time startup message for the MQ135 sensor
     * --------------------------------------------------------------- */
    char msg[100];
    UART_Send("\r\n[MQ135] TASK STARTED!\r\n");

    char startMsg[] = "\r\n----------------------------------------\r\n"
                      "  🚀 Gas  - SERVER ROOM MONITORING \r\n"
                      "----------------------------------------\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)startMsg, strlen(startMsg), HAL_MAX_DELAY);
   // Buzzer_Off();
    for (;;)
    {
        /* ---------------------------------------------------------------
         * STEP 1: Read the MQ135 sensor through the ADC
         * --------------------------------------------------------------- */
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 1000) != HAL_OK)
        {
            UART_Send("[ERROR] ADC conversion failed!\r\n");
            osDelay(1000);
            continue;
        }

        adcValue = HAL_ADC_GetValue(&hadc1);

        /* ---------------------------------------------------------------
         * STEP 2: Convert the raw ADC value into a voltage
         * --------------------------------------------------------------- */
        voltage = ((float)adcValue * 3.3f) / 4095.0f;

        /* ---------------------------------------------------------------
         * STEP 3: Calculate PPM (NEW - added for PrintStatus function)
         * --------------------------------------------------------------- */
        if (voltage < 0.01f) voltage = 0.01f;
        float rs = (10.0f * (3.3f - voltage)) / voltage;
        float ratio = rs / 10.0f;
        ppm = 100.0f * powf(ratio, -1.5f);
        if (ppm < 0.0f) ppm = 0.0f;
        if (ppm > 10000.0f) ppm = 10000.0f;

        /* ---------------------------------------------------------------
         * STEP 4: Classify gas level, light the matching LED,
         *         and append a clear status message with emoji
         * --------------------------------------------------------------- */
        uint8_t gasLevel;  // ← NEW variable for level

        if (adcValue < 1000)
        {
        	strcat(msg, "\r\nGas Status : 🟢LOW GAS\r\n");
            gasLevel = 0;  // ← NEW
            /* LOW -> Green ON, Orange/Red OFF */
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
            //Buzzer_Off();
        }
        else if (adcValue < 2500)
        {
        	strcat(msg, "\r\nGas Status : 🟠 MEDIUM GAS\r\n");
            gasLevel = 1;  // ← NEW
            /* MEDIUM -> Orange ON, Green/Red OFF */

            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
            //Buzzer_Off();
        }
        else
        {
        	strcat(msg, "\r\nGas Status : 🔴 HIGH GAS\r\n");
            gasLevel = 2;  // ← NEW
            /* HIGH -> Red ON, Green/Orange OFF */
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
            //Buzzer_On();
            UART_Send("\r\n🚨🚨🚨 HIGH GAS DETECTED! BUZZER ACTIVATED! 🚨🚨🚨\r\n");
        }

        /* ---------------------------------------------------------------
         * STEP 5: Send the message over UART using PrintStatus function
         * --------------------------------------------------------------- */
        MQ135_PrintStatus(adcValue, voltage, ppm, gasLevel);  // ← REPLACED your old UART code

        /* ---------------------------------------------------------------
         * STEP 6: Wait 1 second before the next reading using FreeRTOS delay
         * --------------------------------------------------------------- */
        osDelay(1000);
    }
}


