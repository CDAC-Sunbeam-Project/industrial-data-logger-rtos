# Industrial Data Logger Using RTOS (STM32)

## 📌 Overview

This project implements a real-time industrial data logging system using STM32 and FreeRTOS. It reads analog sensor data using ADC, logs it into an SD card using SPI, and transmits data to a PC via UART.

---

## ⚙️ Features

* Real-time data acquisition (ADC)
* Multitasking using FreeRTOS
* Data logging to SD card (SPI + FATFS)
* Serial communication via UART

---

## 🔄 System Flow

Sensor → ADC → STM32 → Queue → Logging Task / UART Task

---

## 🔌 Protocols Used

* UART (PC communication)
* SPI (SD card)
* ADC (data acquisition)

---

## 📂 Project Structure (To be added)

* docs/
* code/
* images/

---

## 🚧 Status

Project setup in progress
## ✅ Project Progress
- [x] UART debug communication established
- [x] PIR Motion Sensor (HC-SR501) interfaced via PA2
- [ ] DHT22 Temperature Sensor (In progress)
- [ ] MQ-135 Gas Sensor (Pending)
- [ ] SD Card Logging (Pending)
