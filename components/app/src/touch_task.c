#include "app_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cst816s.h"
#include "bsp_i2c.h"
#include <stdio.h>

static cst816s_t s_touch;

void touch_task(void *arg)
{
    i2c_master_bus_handle_t bus = bsp_i2c_get_bus();

    esp_err_t ret = cst816s_init(&s_touch, bus);
    if (ret != ESP_OK) {
        printf("Touch init failed: 0x%x\r\n", ret);
        vTaskDelete(NULL);
        return;
    }

    printf("Touch init OK\r\n");

    cst816s_point_t point;
    while (1)
    {
        if (cst816s_read(&s_touch, &point) == ESP_OK) {
            xQueueOverwrite(touch_queue, &point);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}