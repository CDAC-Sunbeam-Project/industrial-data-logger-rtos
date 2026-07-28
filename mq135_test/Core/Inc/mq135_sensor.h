/*
 * mq135_sensor.h
 *
 *  Created on: 27-Jul-2026
 *      Author: sunbeam
 */

#ifndef INC_MQ135_SENSOR_H_
#define INC_MQ135_SENSOR_H_

#include "main.h"
#include "cmsis_os.h"

/* MQ135 Configuration */
#define MQ135_ADC_CHANNEL       ADC_CHANNEL_0
#define MQ135_LOW_THRESHOLD     1000
#define MQ135_MEDIUM_THRESHOLD  2500

/* LED Pins */
#define MQ135_LED_GREEN_PIN     GPIO_PIN_12
#define MQ135_LED_ORANGE_PIN    GPIO_PIN_13
#define MQ135_LED_RED_PIN       GPIO_PIN_14

/* Function prototype */
void MQ135_Task(void *argument);


#endif /* INC_MQ135_SENSOR_H_ */
