#include "page_mqtt.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "page_mqtt";

static ui_page_t s_page_mqtt = {
    .name    = "MQTT",
    .screen  = NULL,
    .create  = NULL,
    .destroy = NULL,
    .update  = NULL,
};

static lv_obj_t *s_label_status;
static lv_obj_t *s_label_broker;
static lv_obj_t *s_label_topic;

static void page_mqtt_create(void)
{
    s_page_mqtt.screen = ui_create_screen_scrollable();
    lv_obj_t *scr = s_page_mqtt.screen;

    lv_obj_t *title = ui_create_page_title(scr, "Heart");

    int32_t y_start = ROUND_TOP_OFFSET + 30;

    s_label_status = ui_create_info_card(scr, y_start,
                                         "Status",
                                         LV_SYMBOL_CLOSE " Disconnected",
                                         lv_color_hex(0xFF5252));

    s_label_broker = ui_create_info_card(scr, y_start + 50 + ROUND_PANEL_GAP,
                                         "Broker",
                                         "-",
                                         lv_color_hex(0xAAAAAA));

    s_label_topic  = ui_create_info_card(scr, y_start + 2 * (50 + ROUND_PANEL_GAP),
                                         "Topic",
                                         "-",
                                         lv_color_hex(0xAAAAAA));

    ui_add_gesture_back(scr);

    ESP_LOGI(TAG, "mqtt created");
}

static void page_mqtt_destroy(void)
{
    s_label_status = NULL;
    s_label_broker = NULL;
    s_label_topic = NULL;
    s_page_mqtt.screen = NULL;
    ESP_LOGI(TAG, "mqtt destroyed");
}

static void page_mqtt_update(void)
{
}

ui_page_t *page_mqtt_get(void)
{
    if (!s_page_mqtt.create) {
        s_page_mqtt.create  = page_mqtt_create;
        s_page_mqtt.destroy = page_mqtt_destroy;
        s_page_mqtt.update  = page_mqtt_update;
    }
    return &s_page_mqtt;
}