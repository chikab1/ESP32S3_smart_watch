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
    { "IMU",       0xC80000, LV_SYMBOL_SHUFFLE,  page_imu_get      },
    { "MQTT",      0x009680, LV_SYMBOL_WIFI,     page_mqtt_get     },
    { "About",     0x808080, LV_SYMBOL_HOME,     page_about_get    },
    { "System",    0xDC80E6, LV_SYMBOL_CHARGE,   page_system_get   },
};

#define ITEM_COUNT (sizeof(s_items) / sizeof(s_items[0]))
#define PANEL_HEIGHT 70

static void menu_panel_clicked_cb(lv_event_t *e)
{
    uint32_t idx = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    if (idx >= ITEM_COUNT) return;

    ESP_LOGI(TAG, "clicked: %s", s_items[idx].name);
    ui_page_push(s_items[idx].get_page());
}

static void page_menu_create(void)
{
    s_page_menu.screen = ui_create_screen();
    lv_obj_t *scr = s_page_menu.screen;

    lv_obj_add_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(scr, LV_DIR_VER);

    for (uint32_t i = 0; i < ITEM_COUNT; i++) {
        lv_obj_t *panel = ui_create_menu_panel(scr, (int32_t)i * PANEL_HEIGHT);
        ui_create_menu_icon(panel, lv_color_hex(s_items[i].color_hex), s_items[i].symbol);
        ui_create_menu_label(panel, s_items[i].name);

        lv_obj_add_event_cb(panel, menu_panel_clicked_cb,
                            LV_EVENT_CLICKED, (void *)(uintptr_t)i);
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