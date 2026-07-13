#include "bsp_board.h"
#include "bsp_i2c.h"
#include "lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

void app_main(void)
{
    printf("ESP32-S3 Smart Watch v0.2.0\r\n");

    bsp_init();

    i2c_master_bus_handle_t bus = bsp_i2c_get_bus();

    esp_err_t ret = lvgl_port_init(bus);
    if (ret != ESP_OK) {
        printf("LVGL port init failed: 0x%x\r\n", ret);
        return;
    }

    printf("Watch OS started\r\n");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}