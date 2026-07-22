#include "page_system.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "page_system";

static ui_page_t s_page_system = {
    .name    = "System",
    .screen  = NULL,
    .create  = NULL,
    .destroy = NULL,
    .update  = NULL,
};

static lv_obj_t *s_label_heap;
static lv_obj_t *s_label_tasks;
static lv_obj_t *s_label_uptime;

static void page_system_create(void)
{
    s_page_system.screen = ui_create_screen_scrollable();
    lv_obj_t *scr = s_page_system.screen;

    lv_obj_t *title = ui_create_page_title(scr, "System");

    int32_t y_start = ROUND_TOP_OFFSET + 30;

    s_label_heap = ui_create_info_card(scr, y_start,
                                       "Free Heap",
                                       "-",
                                       lv_color_hex(0xDC80E6));

    s_label_tasks = ui_create_info_card(scr, y_start + 50 + ROUND_PANEL_GAP,
                                        "Tasks",
                                        "-",
                                        lv_color_hex(0xDC80E6));

    s_label_uptime = ui_create_info_card(scr, y_start + 2 * (50 + ROUND_PANEL_GAP),
                                         "Uptime",
                                         "-",
                                         lv_color_hex(0xDC80E6));

    ui_add_gesture_back(scr);

    ESP_LOGI(TAG, "system created");
}

static void page_system_destroy(void)
{
    s_label_heap = NULL;
    s_label_tasks = NULL;
    s_label_uptime = NULL;
    s_page_system.screen = NULL;
    ESP_LOGI(TAG, "system destroyed");
}

static void page_system_update(void)
{
}

ui_page_t *page_system_get(void)
{
    if (!s_page_system.create) {
        s_page_system.create  = page_system_create;
        s_page_system.destroy = page_system_destroy;
        s_page_system.update  = page_system_update;
    }
    return &s_page_system;
}