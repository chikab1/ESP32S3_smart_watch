#include "app_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

QueueHandle_t imu_queue;

void app_start(void)
{
    imu_queue = xQueueCreate(1, sizeof(qmi8658_data_t));

    xTaskCreatePinnedToCore(imu_task, "imu", 4096, NULL, 5, NULL, 0);

    printf("App started: Core0={imu}\r\n");
}