#include "app_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

QueueHandle_t imu_queue;
QueueHandle_t touch_queue;

void app_start(void)
{
    imu_queue = xQueueCreate(1, sizeof(qmi8658_data_t));
    touch_queue = xQueueCreate(1, sizeof(cst816s_point_t));

    xTaskCreatePinnedToCore(imu_task,   "imu",   4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(touch_task, "touch", 4096, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(ui_task,    "ui",    4096, NULL, 3, NULL, 1);

    printf("App started: Core0={imu,touch} Core1={ui}\r\n");
}