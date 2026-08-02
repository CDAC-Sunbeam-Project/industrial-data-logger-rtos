/*
 * bme280_sensor.c
 *
 *  Created on: 24-Jul-2026
 *      Author: sunbeam
 */

#include "bme280_sensor.h"
#include "shared_data.h"
#include <stdio.h>

/* I2C handle declared by CubeMX in main.c */
extern I2C_HandleTypeDef hi2c1;

extern osMutexId_t          dataMutexHandle;
extern osEventFlagsId_t     alertFlagsHandle;

/* ── Private calibration variables ───────────────────────────
 * static = invisible outside this file
 * Read once at boot from sensor's internal flash
 * Never change after BME280_Init() ──────────────────────── */
static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5;
static int16_t  dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1, dig_H3;
static int16_t  dig_H2, dig_H4, dig_H5;
static int8_t   dig_H6;

/* ── Private function declarations ─────────────────────────── */
static uint8_t bme280_check_id(void);
//static uint8_t bme280_read_calibration(void);
static float   bme280_compensate_temp(int32_t adc_T,
                                       int32_t *t_fine);
static float   bme280_compensate_pressure(int32_t adc_P,
                                           int32_t t_fine);
static float   bme280_compensate_humidity(int32_t adc_H,
                                           int32_t t_fine);
static uint8_t bme280_read_raw(float *temp,
                                float *hum,
                                float *pres);

/* ═══════════════════════════════════════════════════════════
   bme280_check_id (private)
   Reads register 0xD0 — must return 0x60 for BME280
   If wrong: sensor not connected or wrong I2C address
═══════════════════════════════════════════════════════════ */
static uint8_t bme280_check_id(void)
{
    uint8_t id = 0;
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        &hi2c1, BME280_I2C_ADDR,
        0xD0, I2C_MEMADD_SIZE_8BIT,
        &id, 1, 100);

    if (status != HAL_OK || id != 0x60)
    {
        printf("[BME280] ERROR: chip ID=0x%02X"
               " expected 0x60, HAL=%d\r\n", id, status);
        printf("[BME280] Check: SDO pin LOW=0x76 HIGH=0x77\r\n");
        return 0;
    }
    printf("[BME280] Chip ID OK: 0x%02X\r\n", id);
    return 1;
}

/* ═══════════════════════════════════════════════════════════
   bme280_read_calibration (private)
   Reads 33 calibration bytes from sensor's internal flash
   Each sensor has unique correction values from factory
   Must be read before any temperature/pressure/humidity calc
═══════════════════════════════════════════════════════════ */
uint8_t bme280_read_calibration(void)
{
    uint8_t c1[26] = {0};
    uint8_t c2[7]  = {0};

    /* Block 1: registers 0x88 to 0xA1 (26 bytes) */
    if (HAL_I2C_Mem_Read(&hi2c1, BME280_I2C_ADDR,
        0x88, I2C_MEMADD_SIZE_8BIT,
        c1, 26, 200) != HAL_OK)
    {
        printf("[BME280] ERROR: calib block 1 failed\r\n");
        return 0;
    }

    /* Little-endian: low byte at lower address */
    dig_T1 = (uint16_t)(c1[1]  << 8 | c1[0]);
    dig_T2 = (int16_t) (c1[3]  << 8 | c1[2]);
    dig_T3 = (int16_t) (c1[5]  << 8 | c1[4]);
    dig_P1 = (uint16_t)(c1[7]  << 8 | c1[6]);
    dig_P2 = (int16_t) (c1[9]  << 8 | c1[8]);
    dig_P3 = (int16_t) (c1[11] << 8 | c1[10]);
    dig_P4 = (int16_t) (c1[13] << 8 | c1[12]);
    dig_P5 = (int16_t) (c1[15] << 8 | c1[14]);
    dig_P6 = (int16_t) (c1[17] << 8 | c1[16]);
    dig_P7 = (int16_t) (c1[19] << 8 | c1[18]);
    dig_P8 = (int16_t) (c1[21] << 8 | c1[20]);
    dig_P9 = (int16_t) (c1[23] << 8 | c1[22]);
    dig_H1 = c1[25];

    /* Block 2: registers 0xE1 to 0xE7 (7 bytes) */
    if (HAL_I2C_Mem_Read(&hi2c1, BME280_I2C_ADDR,
        0xE1, I2C_MEMADD_SIZE_8BIT,
        c2, 7, 200) != HAL_OK)
    {
        printf("[BME280] ERROR: calib block 2 failed\r\n");
        return 0;
    }

    dig_H2 = (int16_t)(c2[1] << 8 | c2[0]);
    dig_H3 = c2[2];
    dig_H4 = (int16_t)(((int8_t)c2[3] << 4) | (c2[4] & 0x0F));
    dig_H5 = (int16_t)(((int8_t)c2[5] << 4) | (c2[4] >> 4));
    dig_H6 = (int8_t)c2[6];

    printf("[BME280] Calibration loaded OK\r\n");
    return 1;
}

/* ── Compensation formulas (private) ──────────────────────────
 * BUG FIX from teammate's original:
 * t_fine was a global variable — dangerous in FreeRTOS
 * because task switch between compensate_temp() and
 * compensate_pressure() would corrupt t_fine value.
 *
 * Fix: t_fine is LOCAL in bme280_read_raw() and passed
 * as parameter to each compensation function.
 * No shared state = completely thread safe.
 ─────────────────────────────────────────────────────────── */
static float bme280_compensate_temp(int32_t adc_T,
                                     int32_t *t_fine)
{
    float var1 = ((float)adc_T / 16384.0f)
               - ((float)dig_T1 / 1024.0f);
    float var2 = var1 * (float)dig_T2;
    var1 = (((float)adc_T / 131072.0f)
          - ((float)dig_T1 / 8192.0f));
    var1 = var1 * var1 * (float)dig_T3;
    *t_fine = (int32_t)(var2 + var1);
    return (var2 + var1) / 5120.0f;
}

static float bme280_compensate_pressure(int32_t adc_P,
                                         int32_t t_fine)
{
    float var1 = ((float)t_fine / 2.0f) - 64000.0f;
    float var2 = var1 * var1 * (float)dig_P6 / 32768.0f;
    var2 = var2 + var1 * (float)dig_P5 * 2.0f;
    var2 = (var2 / 4.0f) + ((float)dig_P4 * 65536.0f);
    var1 = ((float)dig_P3 * var1 * var1 / 524288.0f
          + (float)dig_P2 * var1) / 524288.0f;
    var1 = (1.0f + var1 / 32768.0f) * (float)dig_P1;
    if (var1 == 0.0f) return 0.0f;
    float P = 1048576.0f - (float)adc_P;
    P = (P - (var2 / 4096.0f)) * 6250.0f / var1;
    var1 = (float)dig_P9 * P * P / 2147483648.0f;
    var2 = P * (float)dig_P8 / 32768.0f;
    return (P + (var1 + var2 + (float)dig_P7) / 16.0f)
           / 100.0f;
}

static float bme280_compensate_humidity(int32_t adc_H,
                                         int32_t t_fine)
{
    float x = ((float)t_fine - 76800.0f);
    x = (adc_H - ((float)dig_H4 * 64.0f
        + ((float)dig_H5 / 16384.0f) * x))
        * ((float)dig_H2 / 65536.0f
        * (1.0f + (float)dig_H6 / 67108864.0f * x
        * (1.0f + (float)dig_H3 / 67108864.0f * x)));
    x = x * (1.0f - (float)dig_H1 * x / 524288.0f);
    if (x > 100.0f) x = 100.0f;
    else if (x < 0.0f) x = 0.0f;
    return x;
}

/* ── bme280_read_raw (private) ────────────────────────────── */
static uint8_t bme280_read_raw(float *temp,
                                float *hum,
                                float *pres)
{
    uint8_t raw[8] = {0};

    if (HAL_I2C_Mem_Read(&hi2c1, BME280_I2C_ADDR,
        0xF7, I2C_MEMADD_SIZE_8BIT,
        raw, 8, 200) != HAL_OK)
    {
        printf("[BME280] ERROR: data read failed\r\n");
        return 0;
    }

    int32_t adc_P = (int32_t)((raw[0] << 12)
                             | (raw[1] << 4)
                             | (raw[2] >> 4));
    int32_t adc_T = (int32_t)((raw[3] << 12)
                             | (raw[4] << 4)
                             | (raw[5] >> 4));
    int32_t adc_H = (int32_t)((raw[6] << 8) | raw[7]);

    /* t_fine is LOCAL — lives on this function's stack
     * No other task can touch it = thread safe */
    int32_t t_fine = 0;

    *temp = bme280_compensate_temp    (adc_T, &t_fine);
    *pres = bme280_compensate_pressure(adc_P,  t_fine);
    *hum  = bme280_compensate_humidity(adc_H,  t_fine);
    return 1;
}

/* ═══════════════════════════════════════════════════════════
   BME280_Init (public)
   Called from main() before osKernelStart()
   Returns 1 = success, 0 = sensor not found
═══════════════════════════════════════════════════════════ */
uint8_t BME280_Init(void)
{
	printf("BME280 Task Started\r\n");
    if (!bme280_check_id())          return 0;
    if (!bme280_read_calibration())  return 0;

    /*
     * Register write order MATTERS (datasheet requirement):
     * 1. ctrl_hum (0xF2) — humidity oversampling
     * 2. config   (0xF5) — standby time + IIR filter
     * 3. ctrl_meas(0xF4) — temp/pressure oversampling + mode
     *
     * ctrl_hum only takes effect after ctrl_meas is written
     */
    uint8_t ctrl_hum  = 0x02; /* humidity oversampling x2 */
    uint8_t config    = 0xA0; /* standby 1000ms, IIR filter x4 */
    uint8_t ctrl_meas = 0x6F; /* temp x4, pressure x4, normal */

    if (HAL_I2C_Mem_Write(&hi2c1, BME280_I2C_ADDR,
        0xF2, I2C_MEMADD_SIZE_8BIT,
        &ctrl_hum, 1, 100) != HAL_OK) return 0;

    if (HAL_I2C_Mem_Write(&hi2c1, BME280_I2C_ADDR,
        0xF5, I2C_MEMADD_SIZE_8BIT,
        &config, 1, 100) != HAL_OK) return 0;

    if (HAL_I2C_Mem_Write(&hi2c1, BME280_I2C_ADDR,
        0xF4, I2C_MEMADD_SIZE_8BIT,
        &ctrl_meas, 1, 100) != HAL_OK) return 0;

    HAL_Delay(100);
    printf("[BME280] Init complete\r\n");
    printf("[BME280] Addr: 0x%02X\r\n", BME280_I2C_ADDR >> 1);
    printf("[BME280] Mode: Normal, T/P x4, H x2, IIR x4\r\n\n");
    return 1;
}

/* ═══════════════════════════════════════════════════════════
   BME280_Task (public)
   FreeRTOS task — runs forever
   Priority: osPriorityAboveNormal
   Stack: 1024 bytes (float math needs more than PIR)
═══════════════════════════════════════════════════════════ */
void BME280_Task(void *argument)
{
    printf("[BME280] Task started\r\n\n");

    float temp = 0.0f;
    float hum  = 0.0f;
    float pres = 0.0f;

    for (;;)
    {
        if (bme280_read_raw(&temp, &hum, &pres))
        {
            uint8_t t_alert = (temp > TEMP_MAX_C ||
                               temp < TEMP_MIN_C) ? 1 : 0;
            uint8_t h_alert = (hum  > HUMIDITY_MAX_PCT) ? 1 : 0;

            /* Acquire mutex → write entire bme struct → release
             * Do NOT hold mutex during I2C reads — deadlock risk
             * Read first (above), then lock, write, unlock */
            osMutexAcquire(dataMutexHandle, osWaitForever);
            sysData.bme.temperature    = temp;
            sysData.bme.humidity       = hum;
            sysData.bme.pressure       = pres;
            sysData.bme.temp_alert     = t_alert;
            sysData.bme.humidity_alert = h_alert;
            sysData.bme.sensor_ok      = 1;
            sysData.bme.last_read_ms   = HAL_GetTick();
            sysData.timestamp_ms       = HAL_GetTick();
            osMutexRelease(dataMutexHandle);

            /* Set event flags AFTER releasing mutex */
            if (t_alert)
                osEventFlagsSet(alertFlagsHandle, EVT_TEMP_ALERT);
            if (h_alert)
                osEventFlagsSet(alertFlagsHandle, EVT_HUMIDITY_ALERT);

            printf("[BME280] T=%.2fC  H=%.2f%%  P=%.2fhPa%s\r\n",
                   temp, hum, pres,
                   t_alert ? "  [TEMP ALERT]" : "");
        }
        else
        {
            osMutexAcquire(dataMutexHandle, osWaitForever);
            sysData.bme.sensor_ok = 0;
            osMutexRelease(dataMutexHandle);
        }

        osDelay(2000);
    }
}

