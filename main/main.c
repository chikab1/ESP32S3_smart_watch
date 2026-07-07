#include "bsp_board.h"
#include "bsp_i2c.h"
#include "lvgl_port.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

void app_main(void)
{
    printf("LVGL version: %d.%d.%d\r\n",
           lv_version_major(),
           lv_version_minor(),
           lv_version_patch());

    bsp_init();

    i2c_master_bus_handle_t bus = bsp_i2c_get_bus();

    esp_err_t ret = lvgl_port_init(bus);
    if (ret != ESP_OK) {
        printf("LVGL port init failed: 0x%x\r\n", ret);
        return;
    }

    printf("LVGL port init OK\r\n");

    lvgl_port_lock();

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello LVGL!");
    lv_obj_center(label);

    lv_obj_t *sub = lv_label_create(lv_screen_active());
    lv_label_set_text(sub, "ESP32-S3 Watch");
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 30);

    lvgl_port_unlock();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}