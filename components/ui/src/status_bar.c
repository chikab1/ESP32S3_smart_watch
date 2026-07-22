#include "status_bar.h"
#include "ui_data.h"
#include "wifi_manager.h"
#include "ui_style.h"
#include "lvgl.h"
#include <stdio.h>

static lv_obj_t *s_bar = NULL;
static lv_obj_t *s_icon_wifi = NULL;
static lv_obj_t *s_label_time = NULL;
static lv_obj_t *s_label_bat = NULL;
static lv_timer_t *s_timer = NULL;

static void update_cb(lv_timer_t *timer)
{
    if (!s_bar) return;

    if (wifi_mgr_is_connected()) {
        lv_obj_set_style_text_color(s_icon_wifi, lv_color_hex(0x4FC3F7), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(s_icon_wifi, ui_color_dim(), LV_PART_MAIN);
    }

    ui_datetime_t now = ui_get_time();
    lv_label_set_text_fmt(s_label_time, "%02d:%02d", now.hour, now.minute);

    ui_sensor_data_t s = ui_get_sensor();
    lv_label_set_text_fmt(s_label_bat, LV_SYMBOL_BATTERY_3 " %d%%", s.battery_percent);
}

void status_bar_init(void)
{
    if (s_bar) return;

    s_bar = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_bar);
    lv_obj_set_size(s_bar, 200, 20);
    lv_obj_align(s_bar, LV_ALIGN_TOP_MID, 0, 25);
    lv_obj_set_style_bg_opa(s_bar, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    s_icon_wifi = lv_label_create(s_bar);
    lv_label_set_text(s_icon_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(s_icon_wifi, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_icon_wifi, ui_color_dim(), LV_PART_MAIN);
    lv_obj_align(s_icon_wifi, LV_ALIGN_LEFT_MID, 0, 0);

    s_label_time = lv_label_create(s_bar);
    lv_label_set_text(s_label_time, "--:--");
    lv_obj_set_style_text_font(s_label_time, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_time, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_align(s_label_time, LV_ALIGN_CENTER, 0, 0);

    s_label_bat = lv_label_create(s_bar);
    lv_label_set_text(s_label_bat, LV_SYMBOL_BATTERY_3 " --%");
    lv_obj_set_style_text_font(s_label_bat, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_bat, lv_color_hex(0x81C784), LV_PART_MAIN);
    lv_obj_align(s_label_bat, LV_ALIGN_RIGHT_MID, 0, 0);

    update_cb(NULL);

    s_timer = lv_timer_create(update_cb, 1000, NULL);
    lv_timer_set_repeat_count(s_timer, -1);
}