#include "page_home.h"
#include "page_menu.h"
#include "page_wifi.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
#include "ui_data.h"
#include "quick_settings.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "page_home";

static ui_page_t s_page_home = {
    .name    = "Home",
    .screen  = NULL,
    .create  = NULL,
    .destroy = NULL,
    .update  = NULL,
};

static lv_obj_t *s_label_clock;
static lv_obj_t *s_label_date;
static lv_obj_t *s_label_weekday;
static lv_obj_t *s_label_battery;
static lv_obj_t *s_label_steps;
static lv_timer_t *s_clock_timer;

static ui_datetime_t s_current_time;
static ui_sensor_data_t s_sensor;

static void update_clock_display(void)
{
    lv_label_set_text_fmt(s_label_clock, "%02d:%02d",
                          s_current_time.hour, s_current_time.minute);
    lv_label_set_text_fmt(s_label_date, "%04d-%02d-%02d",
                          s_current_time.year, s_current_time.month,
                          s_current_time.day);
    lv_label_set_text(s_label_weekday,
                      ui_get_weekday_name(s_current_time.weekday));
}

static void update_sensor_display(void)
{
    s_sensor = ui_get_sensor();
    if (s_sensor.battery_present) {
        lv_label_set_text_fmt(s_label_battery, LV_SYMBOL_BATTERY_3 " %d%%",
                              s_sensor.battery_percent);
    } else {
        lv_label_set_text(s_label_battery, LV_SYMBOL_USB " USB");
    }

    lv_label_set_text_fmt(s_label_steps, LV_SYMBOL_SHUFFLE " %d",
                          s_sensor.step_count);
}

static void clock_timer_cb(lv_timer_t *timer)
{
    ui_datetime_t now = ui_get_time();
    if (now.year != s_current_time.year ||
        now.month != s_current_time.month ||
        now.day != s_current_time.day ||
        now.hour != s_current_time.hour ||
        now.minute != s_current_time.minute ||
        now.second != s_current_time.second) {
        s_current_time = now;
        update_clock_display();
    }

    update_sensor_display();
}

static void screen_gesture_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_TOP) {
        ui_page_push(page_menu_get());
    } else if (dir == LV_DIR_BOTTOM) {
        quick_settings_show();
    }
}

static void page_home_create(void)
{
    s_page_home.screen = lv_obj_create(NULL);
    lv_obj_t *scr = s_page_home.screen;

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);

    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(scr, screen_gesture_cb, LV_EVENT_GESTURE, NULL);

    s_label_clock = lv_label_create(scr);
    lv_label_set_text(s_label_clock, "00:00");
    lv_obj_set_style_text_font(s_label_clock, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_clock, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(s_label_clock, LV_ALIGN_CENTER, 0, -10);

    s_label_date = lv_label_create(scr);
    lv_label_set_text(s_label_date, "");
    lv_obj_set_style_text_font(s_label_date, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_date, ui_color_dim(), LV_PART_MAIN);
    lv_obj_align(s_label_date, LV_ALIGN_CENTER, 0, 25);

    s_label_weekday = lv_label_create(scr);
    lv_label_set_text(s_label_weekday, "");
    lv_obj_set_style_text_font(s_label_weekday, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_weekday, ui_color_accent(), LV_PART_MAIN);
    lv_obj_align(s_label_weekday, LV_ALIGN_CENTER, 0, 42);

    s_label_battery = lv_label_create(scr);
    lv_label_set_text(s_label_battery, "");
    lv_obj_set_style_text_font(s_label_battery, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_battery, ui_color_battery(), LV_PART_MAIN);
    lv_obj_align(s_label_battery, LV_ALIGN_CENTER, 0, 65);

    s_label_steps = lv_label_create(scr);
    lv_label_set_text(s_label_steps, "");
    lv_obj_set_style_text_font(s_label_steps, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_steps, ui_color_accent(), LV_PART_MAIN);
    lv_obj_align(s_label_steps, LV_ALIGN_CENTER, 0, 85);

    s_current_time = ui_get_time();
    s_sensor = ui_get_sensor();
    update_clock_display();
    update_sensor_display();

    s_clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);

    ESP_LOGI(TAG, "home created");
}

static void page_home_destroy(void)
{
    if (s_clock_timer) {
        lv_timer_delete(s_clock_timer);
        s_clock_timer = NULL;
    }
    s_label_clock = NULL;
    s_label_date = NULL;
    s_label_weekday = NULL;
    s_label_battery = NULL;
    s_label_steps = NULL;
    s_page_home.screen = NULL;
    ESP_LOGI(TAG, "home destroyed");
}

static void page_home_update(void)
{
}

ui_page_t *page_home_get(void)
{
    if (!s_page_home.create) {
        s_page_home.create  = page_home_create;
        s_page_home.destroy = page_home_destroy;
        s_page_home.update  = page_home_update;
    }
    return &s_page_home;
}