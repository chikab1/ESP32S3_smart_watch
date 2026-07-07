#include "lvgl_port.h"
#include "lcd.h"
#include "cst816s.h"
#include "bsp_board.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static lv_display_t *s_disp;
static lv_indev_t   *s_indev;
static cst816s_t     s_touch;
static SemaphoreHandle_t s_lvgl_mux;

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
    vTaskDelay(1);
}

static void touch_read_cb(lv_indev_t *indev,
                          lv_indev_data_t *data)
{
    cst816s_point_t pt;
    if (cst816s_read(&s_touch, &pt) == ESP_OK && pt.pressed) {
        data->point.x = pt.x;
        data->point.y = pt.y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void lvgl_tick_task(void *arg)
{
    while (1) {
        lv_tick_inc(1);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void lvgl_task(void *arg)
{
    while (1) {
        lvgl_port_lock();
        lv_timer_handler();
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(5));
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

esp_err_t lvgl_port_init(i2c_master_bus_handle_t i2c_bus)
{
    esp_err_t ret = lcd_init();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = cst816s_init(&s_touch, i2c_bus);
    if (ret != ESP_OK) {
        return ret;
    }

    lv_init();

    s_disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_disp, disp_flush_cb);

    size_t buf_size = LCD_WIDTH * 40 * 2;
    void *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    printf("LVGL buf_size=%d buf1=%p\n", buf_size, buf1);
    printf("LVGL %d.%d\n", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR);
    lv_display_set_buffers(s_disp, buf1, NULL, buf_size,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, touch_read_cb);

    s_lvgl_mux = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(lvgl_tick_task, "lv_tick", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(lvgl_task,    "lv_task", 8192, NULL, 2,  NULL, 1);

    return ESP_OK;
}