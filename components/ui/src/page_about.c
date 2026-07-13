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
#define ROW_HEIGHT 50

static void page_about_create(void)
{
    s_page_about.screen = ui_create_screen();
    lv_obj_t *scr = s_page_about.screen;

    for (uint32_t i = 0; i < ENTRY_COUNT; i++) {
        int32_t y = 15 + (int32_t)i * ROW_HEIGHT;

        lv_obj_t *key_label = lv_label_create(scr);
        lv_obj_set_pos(key_label, 20, y);
        lv_label_set_text(key_label, s_entries[i].key);
        lv_obj_set_style_text_color(key_label, ui_color_title(), LV_PART_MAIN);
        lv_obj_set_style_text_font(key_label, &lv_font_montserrat_18, LV_PART_MAIN);

        lv_obj_t *val_label = lv_label_create(scr);
        lv_obj_set_pos(val_label, 25, y + 25);
        lv_label_set_text(val_label, s_entries[i].value);
        lv_obj_set_style_text_color(val_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(val_label, &lv_font_montserrat_14, LV_PART_MAIN);
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