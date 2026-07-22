#include "app_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

void touch_task(void *arg)
{
    printf("touch_task: handled by lvgl_port\r\n");
    vTaskDelete(NULL);
}