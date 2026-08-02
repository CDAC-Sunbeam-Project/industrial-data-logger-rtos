/*
 * mq135_sensor.c
 *
 *  Created on: 27-Jul-2026
 *      Author: sunbeam
 *  Modified: Using sprintf + UART_Send instead of printf for float support
 */

#include "mq135_sensor.h"
#include "shared_data.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include "main.h"
#include <string.h>
#include <math.h>

/* External references */
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;

/* ============================================================
 * SHARED DATA REFERENCES
 * ============================================================ */
extern SystemData_t sysData;
extern osMutexId_t dataMutexHandle;
extern osEventFlagsId_t alertFlagsHandle;

/* Private variables */
static uint32_t adcValue = 0;
static float voltage = 0.0f;
static float ppm = 0.0f;
static uint8_t gasLevel = 0;

static void UART_Send(const char* str)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)str, strlen(str), 1000);
}


/* ============================================================
 * MQ135 Initialization Function
 * ============================================================ */
void MQ135_Init(void)
{
	printf("MQ135 Task Started\r\n");
    char msg[128];

    UART_Send("\r\n[MQ135] Initializing...\r\n");

    /* Start ADC */
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        UART_Send("[MQ135] ERROR: ADC Start Failed!\r\n");
        return;
    }

    /* Perform one conversion */
    if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK)
    {
        UART_Send("[MQ135] ERROR: ADC Conversion Failed!\r\n");
        HAL_ADC_Stop(&hadc1);
        return;
    }

    /* Read ADC Value */
    adcValue = HAL_ADC_GetValue(&hadc1);

    /* Stop ADC */
    HAL_ADC_Stop(&hadc1);

    sprintf(msg,
            "[MQ135] ADC Test OK. Initial ADC = %lu\r\n",
            (unsigned long)adcValue);

    UART_Send(msg);

    UART_Send("[MQ135] Initialization Complete\r\n");
}

//static void MQ135_PrintStatus(uint32_t adcValue, float voltage, float ppm, uint8_t level)
//{
  //  char msg[1024];
    //const char *levelText[] = {"🟢 LOW", "🟠 MEDIUM", "🔴 HIGH"};
//
//    sprintf(msg, "\r\n╔═══════════════════════════════════════════════╗\r\n"
//                 "║     MQ135 Gas Sensor Report                   ║\r\n"
//                 "╠═══════════════════════════════════════════════╣\r\n"
//                 "║  ADC Value  : %4lu                            ║\r\n"
//                 "║  Voltage    : %.2f V                         ║\r\n"
//                 "║  PPM        : %.1f                           ║\r\n"
//                 "║  Status     : %s                             ║\r\n"
//                 "╚═══════════════════════════════════════════════╝\r\n",
//    printf("[MQ135] ADC=%lu\r\n", adcValue);
//    printf("[MQ135] Voltage=%.2f\r\n", voltage);
//    printf("[MQ135] PPM=%.1f\r\n", ppm);
    static void MQ135_PrintStatus(uint32_t adcValue,float voltage,float ppm,uint8_t level)
    {
        printf("[MQ135] Voltage=%.2f PPM=%.1f Level=%d\r\n",voltage, ppm,level);
    }


/**
 * @brief MQ135 FreeRTOS Task - Complete Implementation
 */
void MQ135_Task(void *argument)
{
    (void)argument;

    UART_Send("\r\n[MQ135] TASK STARTED!\r\n");

    char startMsg[] = "\r\n----------------------------------------\r\n"
                      "  🚀 MQ135 Gas Sensor - MONITORING \r\n"
                      "----------------------------------------\r\n";
    printf("Before Delay\r\n");

    osDelay(1000);

    printf("After Delay\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t *)startMsg, strlen(startMsg), HAL_MAX_DELAY);

    for (;;)
    {
        /* STEP 1: Read ADC from PA0 */
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 1000) != HAL_OK)
        {
            UART_Send("[ERROR] ADC conversion failed!\r\n");
            osDelay(1000);
            continue;
        }

        //adcValue = HAL_ADC_GetValue(&hadc1);

        /* STEP 2: Convert ADC to Voltage */
        voltage = ((float)adcValue * 3.3f) / 4095.0f;

        /* STEP 3: Calculate PPM */
        if (voltage < 0.01f) voltage = 0.01f;
        float rs = (10.0f * (3.3f - voltage)) / voltage;
        float ratio = rs / 10.0f;
        ppm = 100.0f * powf(ratio, -1.5f);
        if (ppm < 0.0f) ppm = 0.0f;
        if (ppm > 10000.0f) ppm = 10000.0f;

        /* STEP 4: Classify gas level and control LEDs */
        if (adcValue < 1000)
        {
            gasLevel = 0;  // LOW
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
        }
        else if (adcValue < 2500)
        {
            gasLevel = 1;  // MEDIUM
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
        }
        else
        {
            gasLevel = 2;  // HIGH
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
            UART_Send("\r\n🚨🚨🚨 HIGH GAS DETECTED! 🚨🚨🚨\r\n");
        }

        /* STEP 5: UPDATE SHARED DATA (Mutex Protected) */
        if (dataMutexHandle != NULL)
        {
            osMutexAcquire(dataMutexHandle, osWaitForever);
            sysData.mq135.ppm = ppm;
            sysData.mq135.adc_raw = (uint16_t)adcValue;
            sysData.mq135.sensor_ok = 1;
            sysData.mq135.gas_alert = (gasLevel == 2) ? 1 : 0;
            sysData.timestamp_ms = HAL_GetTick();
            osMutexRelease(dataMutexHandle);
        }

        /* STEP 6: Set Event Flag for Alerts */
        if (gasLevel == 2 && alertFlagsHandle != NULL)
        {
            osEventFlagsSet(alertFlagsHandle, EVT_GAS_ALERT);
        }

        /* STEP 7: Print Status */
        MQ135_PrintStatus(adcValue, voltage, ppm, gasLevel);

        /* STEP 8: Wait 1 second */
        osDelay(1000);
    }
}
