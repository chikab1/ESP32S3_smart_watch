#include "page_about.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "page_about";

static ui_page_t s_page_about = {
    .name    = "About",
    .screen  = NULL,
    .create  = NULL,
    .destroy = NULL,
    .update  = NULL,
};

typedef struct {
    const char *key;
    const char *value;
} about_entry_t;

static const about_entry_t s_entries[] = {
    { "Model",     "ESP32-S3 Watch" },
    { "Firmware",  "v0.2.0"        },
    { "MCU",       "ESP32-S3"       },
    { "OS",        "FreeRTOS"       },
    { "GUI",       "LVGL v9"        },
    { "Flash",     "16 MB"          },
    { "PSRAM",     "8 MB"           },
};

#define ENTRY_COUNT (sizeof(s_entries) / sizeof(s_entries[0]))

static void page_about_create(void)
{
    s_page_about.screen = ui_create_screen_scrollable();
    lv_obj_t *scr = s_page_about.screen;

    lv_obj_t *title = ui_create_page_title(scr, "About");

    int32_t y_start = ROUND_TOP_OFFSET + 30;

    for (uint32_t i = 0; i < ENTRY_COUNT; i++) {
        ui_create_info_card(scr, y_start + (int32_t)i * (50 + ROUND_PANEL_GAP),
                            s_entries[i].key, s_entries[i].value,
                            ui_color_title());
    }

    ui_add_gesture_back(scr);

    ESP_LOGI(TAG, "about created");
}

static void page_about_destroy(void)
{
    s_page_about.screen = NULL;
    ESP_LOGI(TAG, "about destroyed");
}

static void page_about_update(void)
{
}

ui_page_t *page_about_get(void)
{
    if (!s_page_about.create) {
        s_page_about.create  = page_about_create;
        s_page_about.destroy = page_about_destroy;
        s_page_about.update  = page_about_update;
    }
    return &s_page_about;
}