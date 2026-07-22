#include "quick_settings.h"
#include "power_service.h"
#include "settings_service.h"
#include "wifi_manager.h"
#include "ui_style.h"
#include "lvgl.h"
#include <stdio.h>

static lv_obj_t *s_overlay = NULL;
static lv_obj_t *s_slider_bright = NULL;
static lv_obj_t *s_sw_wifi = NULL;
static lv_obj_t *s_sw_wake = NULL;
static lv_obj_t *s_label_bright_val = NULL;

static void delete_overlay(void)
{
    if (s_overlay) {
        lv_obj_del(s_overlay);
        s_overlay = NULL;
        s_slider_bright = NULL;
        s_sw_wifi = NULL;
        s_sw_wake = NULL;
        s_label_bright_val = NULL;
    }
}

static void overlay_click_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (target == s_overlay) {
        quick_settings_hide();
    }
}

static void slider_cb(lv_event_t *e)
{
    int32_t val = lv_slider_get_value(s_slider_bright);
    power_service_set_brightness((uint8_t)val);
    if (s_label_bright_val) {
        lv_label_set_text_fmt(s_label_bright_val, "%d%%", (int)val);
    }
}

static void wifi_sw_cb(lv_event_t *e)
{
    if (lv_obj_has_state(s_sw_wifi, LV_STATE_CHECKED)) {
        wifi_mgr_start_scan();
    } else {
        wifi_mgr_disconnect();
    }
}

static void wake_sw_cb(lv_event_t *e)
{
    bool en = lv_obj_has_state(s_sw_wake, LV_STATE_CHECKED);
    settings_set_wrist_wake(en);
}

static lv_obj_t *create_row(lv_obj_t *parent, const char *label_text)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 170, 32);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, LV_PART_MAIN);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_width(lbl, 70);

    return row;
}

void quick_settings_show(void)
{
    if (s_overlay) return;

    s_overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_overlay);
    lv_obj_set_size(s_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_70, LV_PART_MAIN);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_overlay, overlay_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *panel = lv_obj_create(s_overlay);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, 190, 200);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 12, LV_PART_MAIN);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(panel, 10, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "Quick Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    lv_obj_t *row_bright = create_row(panel, "Bright");
    s_label_bright_val = lv_label_create(row_bright);
    lv_label_set_text_fmt(s_label_bright_val, "%d%%", (int)settings_get_brightness());
    lv_obj_set_style_text_font(s_label_bright_val, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_bright_val, lv_color_hex(0xFFA726), LV_PART_MAIN);
    lv_obj_set_width(s_label_bright_val, 36);
    s_slider_bright = lv_slider_create(row_bright);
    lv_slider_set_range(s_slider_bright, 10, 100);
    lv_slider_set_value(s_slider_bright, settings_get_brightness(), LV_ANIM_OFF);
    lv_obj_set_width(s_slider_bright, 50);
    lv_obj_add_event_cb(s_slider_bright, slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *row_wifi = create_row(panel, "WiFi");
    s_sw_wifi = lv_switch_create(row_wifi);
    lv_obj_set_size(s_sw_wifi, 40, 20);
    if (wifi_mgr_is_connected()) {
        lv_obj_add_state(s_sw_wifi, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(s_sw_wifi, wifi_sw_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *row_wake = create_row(panel, "Wake");
    s_sw_wake = lv_switch_create(row_wake);
    lv_obj_set_size(s_sw_wake, 40, 20);
    if (settings_get_wrist_wake()) {
        lv_obj_add_state(s_sw_wake, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(s_sw_wake, wake_sw_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

void quick_settings_hide(void)
{
    delete_overlay();
}

bool quick_settings_is_visible(void)
{
    return s_overlay != NULL;
}