# Industrial Data Logger Using RTOS (STM32)

## Overview
Real-time industrial server room safety monitor built on
STM32F407 Discovery with FreeRTOS. Simultaneously monitors
unauthorized access, temperature, humidity, air quality,
current draw, and fire detection with instant alerts.

## Hardware
- MCU: STM32F407VGTx Discovery Board
- RTOS: FreeRTOS CMSIS V2
- Clock: 168MHz

## Sensors
| Sensor | Purpose | Interface | Pin |
|--------|---------|-----------|-----|
| HC-SR501 PIR | Motion/entry detection | GPIO | PC2 |
| BME280 | Temperature, Humidity, Pressure | I2C1 | PB6/PB7 |
| MQ135 | Air quality / Gas detection | ADC1 | PA0 |
| INA219 | Current and Voltage monitoring | I2C1 | PB6/PB7 |
| KY026 | Fire detection | GPIO+ADC | PC3/PA1 |
| SD Card | Data logging via FATFS | SPI2 | PB12-15 |
| Buzzer | Audio alerts | GPIO | PB0 |

## FreeRTOS Tasks
| Task | Priority | Stack | Function |
|------|----------|-------|----------|
| Alert_Task | Realtime | 512B | Buzzer alerts on event flags |
| PIR_Task | High | 512B | Edge detect, zone tracking |
| BME280_Task | Above Normal | 1024B | Temp/humidity/pressure |
| Logger_Task | Normal | 2048B | SD card CSV logging |

## Alert Priority
1. Fire - 5 rapid beeps (critical)
2. Gas - 3 beeps (danger)
3. Temperature - 2 beeps (warning)
4. Motion - 1 beep (info)

## Project Structure

    integration/industrial_data_logger/
    Core/
      Inc/
        shared_data.h      - SystemData_t, mutex, flags
        pir_sensor.h
        bme280_sensor.h
      Src/
        main.c             - FreeRTOS init and task creation
        pir_sensor.c       - PIR motion detection
        bme280_sensor.c    - Temperature sensor
    Drivers/               - STM32 HAL
    Middlewares/           - FreeRTOS source

## Branch Strategy
- main — final demo ready code
- dev — stable integrated code (default)
- feature/integration-pir-bme280 — PIR + BME280 integration
- feature/pir — PIR individual development
- feature/bme280 — BME280 individual development
- Feature/MQ135 — MQ135 individual development
- Feature/ina219 — INA219 individual development

## Team
| Member | Contribution |
|--------|-------------|
| Shreya Saste | PIR sensor, FreeRTOS integration, repo management |
| Priyanka Sonje | BME280 temperature sensor |
| Rutuja Kapare | MQ135 gas sensor |
| Riya Pawar | INA219 current sensor |
