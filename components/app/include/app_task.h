#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "qmi8658.h"
#include "cst816s.h"

extern QueueHandle_t imu_queue;
extern QueueHandle_t touch_queue;

void imu_task(void *arg);
void touch_task(void *arg);
void ui_task(void *arg);

void app_start(void);