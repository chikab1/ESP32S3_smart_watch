#include "page_wifi.h"
#include "page_wifi_connect.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
#include "wifi_manager.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdatomic.h>

static const char *TAG = "page_wifi";

#define RESCAN_INTERVAL_MS 15000

static ui_page_t s_page_wifi = {
    .name    = "WiFi",
    .screen  = NULL,
    .create  = NULL,
    .destroy = NULL,
    .update  = NULL,
};

static lv_obj_t *s_label_status;
static lv_obj_t *s_ap_container;
static lv_timer_t *s_scan_timer;
static lv_timer_t *s_rescan_timer;
static volatile atomic_bool s_scan_done_flag;
static volatile atomic_bool s_scanning;

static const char *auth_mode_str(wifi_auth_mode_t mode)
{
    switch (mode) {
    case WIFI_AUTH_OPEN:          return "Open";
    case WIFI_AUTH_WEP:           return "WEP";
    case WIFI_AUTH_WPA_PSK:       return "WPA";
    case WIFI_AUTH_WPA2_PSK:      return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:  return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK:      return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    default:                      return "Secured";
    }
}

static void wifi_panel_clicked_cb(lv_event_t *e)
{
    uint32_t idx = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    uint16_t ap_count = wifi_mgr_get_ap_count();
    if (idx >= ap_count) return;

    const wifi_ap_info_t *list = wifi_mgr_get_ap_list();
    bool is_open = (list[idx].authmode == WIFI_AUTH_OPEN);

    ESP_LOGI(TAG, "clicked: %s (open=%d)", list[idx].ssid, is_open);

    page_wifi_connect_set_target(list[idx].ssid, is_open);
    ui_page_push(page_wifi_connect_get());
}

static void rebuild_ap_list(void)
{
    if (!s_ap_container) return;

    lv_obj_clean(s_ap_container);

    uint16_t ap_count = wifi_mgr_get_ap_count();
    wifi_mgr_state_t state = wifi_mgr_get_state();

    if (state == WIFI_MGR_STATE_SCANNING) {
        lv_obj_t *lbl = lv_label_create(s_ap_container);
        lv_label_set_text(lbl, LV_SYMBOL_REFRESH " Scanning...");
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl, ui_color_dim(), LV_PART_MAIN);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    if (ap_count == 0) {
        lv_obj_t *lbl = lv_label_create(s_ap_container);
        lv_label_set_text(lbl, LV_SYMBOL_WARNING " No WiFi Found");
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl, ui_color_dim(), LV_PART_MAIN);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    const wifi_ap_info_t *list = wifi_mgr_get_ap_list();
    int32_t y = 0;

    for (uint16_t i = 0; i < ap_count; i++) {
        lv_obj_t *panel = lv_obj_create(s_ap_container);
        lv_obj_set_size(panel, ROUND_PANEL_W, ROUND_PANEL_H);
        lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, y);
        lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(panel, ROUND_PANEL_RADIUS, LV_PART_MAIN);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(panel, ui_color_panel_pressed(), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(panel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

        lv_obj_t *icon_btn = lv_button_create(panel);
        lv_obj_set_size(icon_btn, 32, 32);
        lv_obj_set_align(icon_btn, LV_ALIGN_LEFT_MID);
        lv_obj_set_style_margin_left(icon_btn, 8, LV_PART_MAIN);
        lv_obj_remove_flag(icon_btn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(icon_btn, 16, LV_PART_MAIN);
        lv_obj_set_style_bg_color(icon_btn,
            list[i].rssi > -50 ? lv_color_hex(0x19C819) :
            list[i].rssi > -70 ? lv_color_hex(0x3264C8) :
                                 lv_color_hex(0x808080),
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(icon_btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(icon_btn, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(icon_btn, 0, LV_PART_MAIN);

        lv_obj_t *icon_label = lv_label_create(icon_btn);
        lv_obj_set_align(icon_label, LV_ALIGN_CENTER);
        lv_label_set_text(icon_label, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_color(icon_label, lv_color_white(), LV_PART_MAIN);

        lv_obj_t *name_label = lv_label_create(panel);
        lv_obj_set_align(name_label, LV_ALIGN_LEFT_MID);
        lv_obj_set_pos(name_label, 50, -6);
        lv_label_set_text(name_label, list[i].ssid);
        lv_obj_set_style_text_color(name_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(name_label, &lv_font_montserrat_14, LV_PART_MAIN);

        char detail_str[24];
        snprintf(detail_str, sizeof(detail_str), "%d dBm  %s",
                 list[i].rssi, auth_mode_str(list[i].authmode));
        lv_obj_t *detail_label = lv_label_create(panel);
        lv_obj_set_align(detail_label, LV_ALIGN_LEFT_MID);
        lv_obj_set_pos(detail_label, 50, 8);
        lv_label_set_text(detail_label, detail_str);
        lv_obj_set_style_text_color(detail_label, ui_color_dim(), LV_PART_MAIN);
        lv_obj_set_style_text_font(detail_label, &lv_font_montserrat_14, LV_PART_MAIN);

        if (list[i].connected) {
            lv_obj_t *check = lv_label_create(panel);
            lv_obj_set_align(check, LV_ALIGN_RIGHT_MID);
            lv_obj_set_style_margin_right(check, 10, LV_PART_MAIN);
            lv_label_set_text(check, LV_SYMBOL_OK);
            lv_obj_set_style_text_color(check, lv_color_hex(0x19C819), LV_PART_MAIN);
            lv_obj_set_style_text_font(check, &lv_font_montserrat_14, LV_PART_MAIN);
        }

        lv_obj_add_event_cb(panel, wifi_panel_clicked_cb,
                            LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        y += ROUND_PANEL_H + ROUND_PANEL_GAP;
    }
}

static void update_status_display(void)
{
    if (!s_label_status) return;

    wifi_mgr_state_t state = wifi_mgr_get_state();
    const char *ssid = wifi_mgr_get_connected_ssid();

    switch (state) {
    case WIFI_MGR_STATE_SCANNING:
        lv_label_set_text(s_label_status, LV_SYMBOL_REFRESH " Scanning...");
        lv_obj_set_style_text_color(s_label_status, lv_color_hex(0xFFAA00), LV_PART_MAIN);
        break;
    case WIFI_MGR_STATE_CONNECTED:
        if (ssid) {
            char buf[48];
            snprintf(buf, sizeof(buf), LV_SYMBOL_OK " %s", ssid);
            lv_label_set_text(s_label_status, buf);
        } else {
            lv_label_set_text(s_label_status, LV_SYMBOL_OK " Connected");
        }
        lv_obj_set_style_text_color(s_label_status, lv_color_hex(0x19C819), LV_PART_MAIN);
        break;
    case WIFI_MGR_STATE_CONNECTING:
        lv_label_set_text(s_label_status, LV_SYMBOL_REFRESH " Connecting...");
        lv_obj_set_style_text_color(s_label_status, lv_color_hex(0xFFAA00), LV_PART_MAIN);
        break;
    default:
        lv_label_set_text(s_label_status, LV_SYMBOL_CLOSE " Disconnected");
        lv_obj_set_style_text_color(s_label_status, lv_color_hex(0xFF5252), LV_PART_MAIN);
        break;
    }
}

static void scan_done_cb(void)
{
    atomic_store(&s_scan_done_flag, true);
    atomic_store(&s_scanning, false);
}

static void scan_timer_cb(lv_timer_t *timer)
{
    bool done = atomic_exchange(&s_scan_done_flag, false);
    if (done) {
        update_status_display();
        rebuild_ap_list();
        if (s_scan_timer) {
            lv_timer_del(s_scan_timer);
            s_scan_timer = NULL;
        }
    }
}

static void rescan_timer_cb(lv_timer_t *timer)
{
    if (atomic_load(&s_scanning)) return;

    atomic_store(&s_scanning, true);
    wifi_mgr_start_scan();

    if (!s_scan_timer) {
        s_scan_timer = lv_timer_create(scan_timer_cb, 200, NULL);
    }
}

static lv_obj_t *create_status_panel(lv_obj_t *parent, int32_t y)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, ROUND_SAFE_W, 36);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_set_style_radius(panel, ROUND_PANEL_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(panel, 12, LV_PART_MAIN);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    s_label_status = lv_label_create(panel);
    lv_obj_set_align(s_label_status, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_text_font(s_label_status, &lv_font_montserrat_14, LV_PART_MAIN);

    return panel;
}

static void page_wifi_create(void)
{
    atomic_store(&s_scan_done_flag, false);
    atomic_store(&s_scanning, false);

    s_page_wifi.screen = ui_create_screen_scrollable();
    lv_obj_t *scr = s_page_wifi.screen;

    ui_create_page_title(scr, "WiFi");

    int32_t y_start = ROUND_TOP_OFFSET + 30;

    create_status_panel(scr, y_start);
    update_status_display();

    y_start += 36 + ROUND_PANEL_GAP;

    s_ap_container = lv_obj_create(scr);
    lv_obj_set_size(s_ap_container, ROUND_SCREEN_SIZE, LV_SIZE_CONTENT);
    lv_obj_align(s_ap_container, LV_ALIGN_TOP_MID, 0, y_start);
    lv_obj_set_style_bg_opa(s_ap_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_ap_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_ap_container, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_ap_container, LV_OBJ_FLAG_CLICKABLE |
                        LV_OBJ_FLAG_SCROLLABLE);

    if (wifi_mgr_get_ap_count() > 0) {
        rebuild_ap_list();
    }

    wifi_mgr_set_scan_done_cb(scan_done_cb);
    wifi_mgr_start_scan();
    atomic_store(&s_scanning, true);

    s_scan_timer = lv_timer_create(scan_timer_cb, 200, NULL);
    s_rescan_timer = lv_timer_create(rescan_timer_cb, RESCAN_INTERVAL_MS, NULL);

    ui_add_gesture_back(scr);

    ESP_LOGI(TAG, "wifi created");
}

static void page_wifi_destroy(void)
{
    if (s_scan_timer) {
        lv_timer_del(s_scan_timer);
        s_scan_timer = NULL;
    }
    if (s_rescan_timer) {
        lv_timer_del(s_rescan_timer);
        s_rescan_timer = NULL;
    }
    wifi_mgr_set_scan_done_cb(NULL);
    s_label_status = NULL;
    s_ap_container = NULL;
    s_page_wifi.screen = NULL;
    ESP_LOGI(TAG, "wifi destroyed");
}

static void page_wifi_update(void)
{
}

ui_page_t *page_wifi_get(void)
{
    if (!s_page_wifi.create) {
        s_page_wifi.create  = page_wifi_create;
        s_page_wifi.destroy = page_wifi_destroy;
        s_page_wifi.update  = page_wifi_update;
    }
    return &s_page_wifi;
}