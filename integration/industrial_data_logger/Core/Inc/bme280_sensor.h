/*
 * bme280_sensor.h
 *
 *  Created on: 24-Jul-2026
 *      Author: sunbeam
 */

#ifndef INC_BME280_SENSOR_H_
#define INC_BME280_SENSOR_H_

#include "main.h"
#include "shared_data.h"
#include "cmsis_os.h"

/* ── I2C address ─────────────────────────────────────────── */
#define BME280_I2C_ADDR    (0x76 << 1)
//#define BME280_ADDR         (0x76 << 1)

/* Public API */
uint8_t BME280_Init(void);

//BME280_Data_t BME280_ReadData(void);
void BME280_Task(void *argument);


#endif /* INC_BME280_SENSOR_H_ */
