/*
 * pir_sensor.h
 *
 *  Created on: 24-Jul-2026
 *      Author: sunbeam
 */

#ifndef INC_PIR_SENSOR_H_
#define INC_PIR_SENSOR_H_


#include "main.h"
#include "shared_data.h"

/* Public API — only these two functions visible outside */
void PIR_Init(void);
void PIR_Task(void *argument);

#endif /* INC_PIR_SENSOR_H_ */
