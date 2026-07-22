#include "bsp_board.h"
#include "bsp_i2c.h"
#include "lvgl_port.h"
#include "wifi_manager.h"
#include "battery.h"
#include "app_task.h"
#include "watch_event.h"
#include "power_service.h"
#include "settings_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>

void app_main(void)
{
    printf("ESP32-S3 Smart Watch v0.5.0\r\n");

    bsp_init();
    battery_adc_init();

    nvs_flash_init();
    settings_init();
    watch_event_init();
    power_service_init();

    i2c_master_bus_handle_t bus = bsp_i2c_get_bus();

    app_start();

    esp_err_t ret = lvgl_port_init(bus);
    if (ret != ESP_OK) {
        printf("LVGL port init failed: 0x%x\r\n", ret);
        return;
    }

    wifi_mgr_init();

    printf("Watch OS started\r\n");
}