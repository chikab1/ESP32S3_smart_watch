#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "qmi8658.h"

extern QueueHandle_t imu_queue;

void imu_task(void *arg);

void app_start(void);

qmi8658_t *imu_get_handle(void);

void imu_deinit(void);

void imu_init_after_wakeup(void);