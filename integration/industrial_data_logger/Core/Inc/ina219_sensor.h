/*
 * ina219_sensor.h
 *
 *  Created on: 31-Jul-2026
 *      Author: sunbeam
 */

#ifndef INC_INA219_SENSOR_H_
#define INC_INA219_SENSOR_H_

/* INA219 I2C Address */
#define INA219_ADDR (0x40 << 1)

/* INA219 Registers */
#define INA219_REG_CONFIG   0x00
#define INA219_REG_SHUNT    0x01
#define INA219_REG_BUS      0x02
#define INA219_REG_POWER    0x03
#define INA219_REG_CURRENT  0x04
#define INA219_REG_CALIB    0x05

/* Function Prototypes */
void INA219_Init(void);
void INA219_Task(void *argument);



#endif /* INC_INA219_SENSOR_H_ */
