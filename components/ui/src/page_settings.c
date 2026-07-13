#include "page_settings.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "page_settings";

static ui_page_t s_page_settings = {
    .name    = "Settings",
    .screen  = NULL,
    .create  = NULL,
    .destroy = NULL,
    .update  = NULL,
};

typedef struct {
    const char *name;
    uint32_t color_hex;
    const char *symbol;
    bool has_switch;
} settings_item_t;

static const settings_item_t s_items[] = {
    { "Brightness",  0x0080FF, LV_SYMBOL_IMAGE,    false },
    { "Screen Time", 0xE18019, LV_SYMBOL_PREV,     false },
    { "Date & Time", 0x0080FF, LV_SYMBOL_SETTINGS, false },
    { "Wrist Wake",  0x7D197D, LV_SYMBOL_BELL,     true  },
    { "Password",    0xE18019, LV_SYMBOL_OK,       false },
};

#define ITEM_COUNT (sizeof(s_items) / sizeof(s_items[0]))
#define PANEL_HEIGHT 70

static void page_settings_create(void)
{
    s_page_settings.screen = ui_create_screen();
    lv_obj_t *scr = s_page_settings.screen;

    for (uint32_t i = 0; i < ITEM_COUNT; i++) {
        lv_obj_t *panel = ui_create_menu_panel(scr, (int32_t)i * PANEL_HEIGHT);
        ui_create_menu_icon(panel, lv_color_hex(s_items[i].color_hex), s_items[i].symbol);
        ui_create_menu_label(panel, s_items[i].name);

        if (s_items[i].has_switch) {
            lv_obj_t *sw = lv_switch_create(panel);
            lv_obj_set_size(sw, 55, 25);
            lv_obj_set_align(sw, LV_ALIGN_RIGHT_MID);
            lv_obj_set_style_margin_right(sw, 10, LV_PART_MAIN);
        }
    }

    ui_add_gesture_back(scr);

    ESP_LOGI(TAG, "settings created");
}

static void page_settings_destroy(void)
{
    s_page_settings.screen = NULL;
    ESP_LOGI(TAG, "settings destroyed");
}

static void page_settings_update(void)
{
}

ui_page_t *page_settings_get(void)
{
    if (!s_page_settings.create) {
        s_page_settings.create  = page_settings_create;
        s_page_settings.destroy = page_settings_destroy;
        s_page_settings.update  = page_settings_update;
    }
    return &s_page_settings;
}