#include "page_menu.h"
#include "page_settings.h"
#include "page_imu.h"
#include "page_mqtt.h"
#include "page_about.h"
#include "page_system.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "page_menu";

static ui_page_t s_page_menu = {
    .name    = "Menu",
    .screen  = NULL,
    .create  = NULL,
    .destroy = NULL,
    .update  = NULL,
};

typedef struct {
    const char *name;
    uint32_t color_hex;
    const char *symbol;
    ui_page_t *(*get_page)(void);
} menu_item_t;

static const menu_item_t s_items[] = {
    { "Settings",  0x3264C8, LV_SYMBOL_SETTINGS, page_settings_get },
    { "IMU",       0xE11432, LV_SYMBOL_IMAGE,    page_imu_get      },
    { "Heart",     0xDC80E6, LV_SYMBOL_BELL,     page_mqtt_get     },
    { "Weather",   0x14C8E1, LV_SYMBOL_TINT,     page_about_get    },
    { "About",     0x808080, LV_SYMBOL_HOME,     page_system_get   },
};

#define ITEM_COUNT (sizeof(s_items) / sizeof(s_items[0]))

static void menu_panel_clicked_cb(lv_event_t *e)
{
    uint32_t idx = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    if (idx >= ITEM_COUNT) return;

    ESP_LOGI(TAG, "clicked: %s", s_items[idx].name);
    ui_page_push(s_items[idx].get_page());
}

static void page_menu_create(void)
{
    s_page_menu.screen = ui_create_screen_scrollable();
    lv_obj_t *scr = s_page_menu.screen;

    lv_obj_t *title = ui_create_page_title(scr, "Menu");

    int32_t y_start = ROUND_TOP_OFFSET + 30;
    int32_t y = y_start;

    for (uint32_t i = 0; i < ITEM_COUNT; i++) {
        lv_obj_t *panel = ui_create_menu_panel(scr, y);
        ui_create_menu_icon(panel, lv_color_hex(s_items[i].color_hex), s_items[i].symbol);
        ui_create_menu_label(panel, s_items[i].name);

        lv_obj_add_event_cb(panel, menu_panel_clicked_cb,
                            LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        y += ROUND_PANEL_H + ROUND_PANEL_GAP;
    }

    ui_add_gesture_back(scr);

    ESP_LOGI(TAG, "menu created, %d items", (int)ITEM_COUNT);
}

static void page_menu_destroy(void)
{
    s_page_menu.screen = NULL;
    ESP_LOGI(TAG, "menu destroyed");
}

static void page_menu_update(void)
{
}

ui_page_t *page_menu_get(void)
{
    if (!s_page_menu.create) {
        s_page_menu.create  = page_menu_create;
        s_page_menu.destroy = page_menu_destroy;
        s_page_menu.update  = page_menu_update;
    }
    return &s_page_menu;
}