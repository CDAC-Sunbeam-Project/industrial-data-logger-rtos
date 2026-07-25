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

/* Public API */
uint8_t BME280_Init(void);
void    BME280_Task(void *argument);


#endif /* INC_BME280_SENSOR_H_ */
