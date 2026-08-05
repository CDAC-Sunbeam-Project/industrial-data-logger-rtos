#include "uart_task.h"
#include "shared_data.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart2;

void UART_Task(void *argument)
{
    char txBuffer[200];
    SystemData_t snap;

    printf("[UART] Task Started\r\n");

    for(;;)
    {
        osMutexAcquire(dataMutexHandle, osWaitForever);
        snap = sysData;
        osMutexRelease(dataMutexHandle);

        sprintf(txBuffer,
        "%.2f,%.2f,%.2f,"
        "%.2f,"
        "%.2f,%.2f,%.2f,"
        "%d,%lu,%lu,%lu\r\n",

        snap.bme.temperature,
        snap.bme.humidity,
        snap.bme.pressure,

        snap.mq135.ppm,

        snap.ina.voltage_V,
        snap.ina.current_A,
        snap.ina.power_W,

        snap.pir.zone_occupied,
        snap.pir.event_count,
        snap.pir.zone_entry_time_ms,
        snap.pir.zone_duration_min);

        osMutexAcquire(uartMutexHandle, osWaitForever);
                HAL_UART_Transmit(&huart2,
                                  (uint8_t *)txBuffer,
                                  strlen(txBuffer),
                                  HAL_MAX_DELAY);
                osMutexRelease(uartMutexHandle);

                osDelay(3000);
    }
}
