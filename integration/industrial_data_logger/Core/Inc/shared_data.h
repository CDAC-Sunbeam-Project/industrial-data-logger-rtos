/*
 * shared_data.h
 *
 *  Created on: 24-Jul-2026
 *      Author: sunbeam
 */

#ifndef INC_SHARED_DATA_H_
#define INC_SHARED_DATA_H_

#include "stdint.h"
#include "cmsis_os.h"

/* ── PIR Data ─────────────────────────────────────────────── */
typedef struct {
    uint8_t  zone_occupied;
    uint32_t event_count;
    uint32_t zone_entry_time_ms;
    uint32_t zone_duration_min;
    uint8_t  sensor_ok;
} PIR_Data_t;

/* ── BME280 Data ──────────────────────────────────────────── */
typedef struct {
    float    temperature;
    float    humidity;
    float    pressure;
    uint8_t  temp_alert;
    uint8_t  humidity_alert;
    uint8_t  sensor_ok;
    uint32_t last_read_ms;
} BME280_Data_t;

/* ── Fire Sensor Data ─────────────────────────────────────── */
typedef struct {
    uint16_t adc_raw;
    uint8_t  fire_confirmed;
    uint8_t  sensor_ok;
} Fire_Data_t;

/* ── MQ135 Gas Sensor Data ────────────────────────────────── */
typedef struct {
    float    ppm;
    uint16_t adc_raw;
    uint8_t  gas_alert;
    uint8_t  sensor_ok;
} MQ135_Data_t;

/* ── INA219 Current Sensor Data ───────────────────────────── */
typedef struct {
    float   current_A;
    float   voltage_V;
    float   power_W;
    uint8_t overcurrent_alert;
    uint8_t sensor_ok;
} INA219_Data_t;

/* ── Master System Struct ─────────────────────────────────── */
typedef struct {
    PIR_Data_t    pir;
    BME280_Data_t bme;
    Fire_Data_t   fire;
    MQ135_Data_t  mq135;
    INA219_Data_t ina;
    uint32_t      timestamp_ms;
    uint32_t      uptime_s;
    uint8_t       system_ok;
} SystemData_t;

/* ── Extern — defined once in main.c ─────────────────────── */
extern SystemData_t      sysData;
extern osMutexId_t       dataMutexHandle;
extern osMutexId_t       uartMutexHandle;
extern osEventFlagsId_t  alertFlagsHandle;

/* ── Event Flags ──────────────────────────────────────────── */
#define EVT_MOTION_DETECTED    (1UL << 0)
#define EVT_ZONE_CLEARED       (1UL << 1)
#define EVT_FIRE_ALERT         (1UL << 2)
#define EVT_TEMP_ALERT         (1UL << 3)
#define EVT_HUMIDITY_ALERT     (1UL << 4)
#define EVT_GAS_ALERT          (1UL << 5)
#define EVT_OVERCURRENT        (1UL << 6)

/* ── Thresholds ───────────────────────────────────────────── */
#define TEMP_MAX_C             40.0f
#define TEMP_MIN_C              0.0f
#define HUMIDITY_MAX_PCT       70.0f
#define GAS_PPM_MAX          1000.0f
#define CURRENT_MAX_A           5.0f
#define FIRE_CONFIRM_COUNT         5

/* ── PIR Timing ───────────────────────────────────────────── */
#define PIR_WARMUP_MS          30000UL
#define PIR_DEBOUNCE_MS           50UL
#define PIR_RECHECK_MS        120000UL
#define PIR_POLL_MS               50UL

#endif /* INC_SHARED_DATA_H_ */
