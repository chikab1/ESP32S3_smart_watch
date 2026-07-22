#include "lvgl_port.h"
#include "ui_manager.h"
#include "ui_data.h"
#include "lcd.h"
#include "cst816s.h"
#include "bsp_board.h"
#include "power_service.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2c_master.h"
#include <string.h>

static const char *TAG = "lvgl_port";

static lv_display_t *s_disp;
static lv_indev_t   *s_indev;
static cst816s_t     s_touch;
static SemaphoreHandle_t s_lvgl_mux;
static volatile bool s_lvgl_suspended = false;

static void disp_flush_cb(lv_display_t *disp,
                          const lv_area_t *area,
                          uint8_t *px_map)
{
    uint16_t w = area->x2 - area->x1 + 1;
    uint16_t h = area->y2 - area->y1 + 1;
    uint32_t px_cnt = (uint32_t)w * h;
    uint16_t *px = (uint16_t *)px_map;

    for (uint32_t i = 0; i < px_cnt; i++) {
        px[i] = (px[i] >> 8) | (px[i] << 8);
    }

    lcd_draw_bitmap(area->x1, area->y1, w, h, px_map);
    lv_display_flush_ready(disp);
}

static bool s_last_pressed = false;

static void touch_read_cb(lv_indev_t *indev,
                          lv_indev_data_t *data)
{
    cst816s_point_t pt;
    esp_err_t ret = cst816s_read(&s_touch, &pt);

    if (ret != ESP_OK) {
        data->state = s_last_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
        return;
    }

    if (pt.pressed) {
        data->point.x = pt.x;
        data->point.y = pt.y;
        data->state   = LV_INDEV_STATE_PRESSED;
        if (!s_last_pressed) {
            s_last_pressed = true;
            power_service_reset_idle_timer();
        }
    } else {
        if (s_last_pressed) {
            s_last_pressed = false;
        }
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static uint32_t my_tick_get_cb(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "lvgl_task started on core %d", xPortGetCoreID());

    while (1) {
        if (s_lvgl_suspended) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        power_state_t pwr = power_service_get_state();

        if (pwr == POWER_STATE_SCREEN_OFF) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        xSemaphoreTake(s_lvgl_mux, portMAX_DELAY);
        lv_timer_handler();
        xSemaphoreGive(s_lvgl_mux);

        ui_data_poll_imu();
        ui_update();

        if (pwr == POWER_STATE_ACTIVE) {
            vTaskDelay(pdMS_TO_TICKS(5));
        } else {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

void lvgl_port_lock(void)
{
    xSemaphoreTake(s_lvgl_mux, portMAX_DELAY);
}

void lvgl_port_unlock(void)
{
    xSemaphoreGive(s_lvgl_mux);
}

void lvgl_port_suspend(void)
{
    s_lvgl_suspended = true;
    ESP_LOGI(TAG, "LVGL suspended");
}

void lvgl_port_resume(void)
{
    s_lvgl_suspended = false;
    lv_refr_now(s_disp);
    ESP_LOGI(TAG, "LVGL resumed");
}

esp_err_t lvgl_port_init(i2c_master_bus_handle_t i2c_bus)
{
    esp_err_t ret = lcd_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "lcd_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = cst816s_init(&s_touch, i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "cst816s_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "lcd+touch init OK");

    lv_init();

    lv_tick_set_cb(my_tick_get_cb);

    s_disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_disp, disp_flush_cb);

    size_t buf_size = LCD_WIDTH * 40 * 2;
    void *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    lv_display_set_buffers(s_disp, buf1, NULL, buf_size,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, touch_read_cb);
    lv_indev_set_gesture_min_distance(s_indev, 30);
    lv_indev_set_gesture_min_velocity(s_indev, 2);

    s_lvgl_mux = xSemaphoreCreateMutex();

    lvgl_port_lock();
    ui_init();
    lvgl_port_unlock();

    ESP_LOGI(TAG, "init done, heap=%u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    xTaskCreatePinnedToCore(lvgl_task, "lv_task", 8192, NULL, 2, NULL, tskNO_AFFINITY);

    return ESP_OK;
}

cst816s_t *lvgl_port_get_touch(void)
{
    return &s_touch;
}