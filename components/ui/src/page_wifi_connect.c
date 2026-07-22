#include "page_wifi_connect.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
#include "wifi_manager.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>

static const char *TAG = "page_wifi_conn";

static ui_page_t s_page_wifi_conn = {
    .name    = "WiFiConnect",
    .screen  = NULL,
    .create  = NULL,
    .destroy = NULL,
    .update  = NULL,
};

static char s_target_ssid[33];
static bool s_target_is_open;
static lv_obj_t *s_textarea_pwd;
static lv_obj_t *s_keyboard;
static lv_obj_t *s_btn_connect;
static lv_obj_t *s_label_status;
static lv_timer_t *s_conn_timer;
static volatile atomic_bool s_conn_done_flag;
static volatile atomic_bool s_conn_success_flag;

static void conn_result_cb(wifi_mgr_conn_result_t result)
{
    if (result == WIFI_MGR_CONN_SUCCESS) {
        atomic_store(&s_conn_success_flag, true);
    }
    atomic_store(&s_conn_done_flag, true);
    ESP_LOGI(TAG, "conn result: %s", result == WIFI_MGR_CONN_SUCCESS ? "OK" : "FAIL");
}

static void conn_timer_cb(lv_timer_t *timer);

static void connect_clicked_cb(lv_event_t *e)
{
    const char *pwd = s_target_is_open ? "" : lv_textarea_get_text(s_textarea_pwd);

    if (!s_target_is_open && strlen(pwd) == 0) {
        lv_label_set_text(s_label_status, "Password required");
        lv_obj_set_style_text_color(s_label_status, lv_color_hex(0xFF5252), LV_PART_MAIN);
        return;
    }

    lv_label_set_text(s_label_status, LV_SYMBOL_REFRESH " Connecting...");
    lv_obj_set_style_text_color(s_label_status, lv_color_hex(0xFFAA00), LV_PART_MAIN);
    lv_obj_add_state(s_btn_connect, LV_STATE_DISABLED);

    if (s_keyboard) {
        lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    }

    atomic_store(&s_conn_done_flag, false);
    atomic_store(&s_conn_success_flag, false);

    wifi_mgr_connect(s_target_ssid, pwd, conn_result_cb);

    s_conn_timer = lv_timer_create(conn_timer_cb, 200, NULL);
}

static void conn_timer_cb(lv_timer_t *timer)
{
    bool done = atomic_exchange(&s_conn_done_flag, false);
    if (!done) return;

    bool success = atomic_exchange(&s_conn_success_flag, false);

    if (success) {
        lv_label_set_text(s_label_status, LV_SYMBOL_OK " Connected!");
        lv_obj_set_style_text_color(s_label_status, lv_color_hex(0x19C819), LV_PART_MAIN);
    } else {
        lv_label_set_text(s_label_status, LV_SYMBOL_CLOSE " Auth failed");
        lv_obj_set_style_text_color(s_label_status, lv_color_hex(0xFF5252), LV_PART_MAIN);
        lv_obj_clear_state(s_btn_connect, LV_STATE_DISABLED);
    }

    if (s_conn_timer) {
        lv_timer_del(s_conn_timer);
        s_conn_timer = NULL;
    }
}

static void ta_focus_cb(lv_event_t *e)
{
    if (!s_keyboard) return;
    lv_obj_t *ta = lv_event_get_target(e);
    lv_keyboard_set_textarea(s_keyboard, ta);
    lv_obj_clear_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    if (s_btn_connect) lv_obj_add_flag(s_btn_connect, LV_OBJ_FLAG_HIDDEN);
}

static void ta_defocus_cb(lv_event_t *e)
{
    if (!s_keyboard) return;
    lv_keyboard_set_textarea(s_keyboard, NULL);
    lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    if (s_btn_connect) lv_obj_clear_flag(s_btn_connect, LV_OBJ_FLAG_HIDDEN);
}

static void kb_ready_cb(lv_event_t *e)
{
    if (!s_keyboard || !s_textarea_pwd) return;
    lv_keyboard_set_textarea(s_keyboard, NULL);
    lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    if (s_btn_connect) lv_obj_clear_flag(s_btn_connect, LV_OBJ_FLAG_HIDDEN);
}

static void page_wifi_conn_create(void)
{
    s_page_wifi_conn.screen = ui_create_screen();
    lv_obj_t *scr = s_page_wifi_conn.screen;

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_pos(title, 0, ROUND_TOP_OFFSET);
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_label_set_text(title, s_target_ssid);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);

    int32_t y = ROUND_TOP_OFFSET + 28;

    if (s_target_is_open) {
        lv_obj_t *lbl = lv_label_create(scr);
        lv_obj_set_pos(lbl, 0, y);
        lv_obj_set_align(lbl, LV_ALIGN_TOP_MID);
        lv_label_set_text(lbl, LV_SYMBOL_WIFI " Open Network");
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl, ui_color_dim(), LV_PART_MAIN);
        y += 25;
    } else {
        lv_obj_t *lbl_pwd = lv_label_create(scr);
        lv_obj_set_pos(lbl_pwd, ROUND_SAFE_MARGIN, y);
        lv_label_set_text(lbl_pwd, "Password");
        lv_obj_set_style_text_font(lbl_pwd, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_pwd, ui_color_dim(), LV_PART_MAIN);
        y += 20;

        s_textarea_pwd = lv_textarea_create(scr);
        lv_obj_set_size(s_textarea_pwd, ROUND_SAFE_W, 36);
        lv_obj_set_pos(s_textarea_pwd, ROUND_SAFE_MARGIN, y);
        lv_textarea_set_placeholder_text(s_textarea_pwd, "Enter password");
        lv_textarea_set_password_mode(s_textarea_pwd, false);
        lv_textarea_set_one_line(s_textarea_pwd, true);
        lv_textarea_set_max_length(s_textarea_pwd, 63);
        lv_obj_set_style_text_font(s_textarea_pwd, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_textarea_pwd, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(s_textarea_pwd, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(s_textarea_pwd, lv_color_hex(0x3264C8), LV_PART_MAIN);
        lv_obj_set_style_border_width(s_textarea_pwd, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(s_textarea_pwd, 8, LV_PART_MAIN);
        lv_obj_add_event_cb(s_textarea_pwd, ta_focus_cb, LV_EVENT_FOCUSED, NULL);
        lv_obj_add_event_cb(s_textarea_pwd, ta_defocus_cb, LV_EVENT_DEFOCUSED, NULL);
        y += 44;
    }

    s_label_status = lv_label_create(scr);
    lv_obj_set_pos(s_label_status, 0, y);
    lv_obj_set_align(s_label_status, LV_ALIGN_TOP_MID);
    lv_label_set_text(s_label_status, "");
    lv_obj_set_style_text_font(s_label_status, &lv_font_montserrat_14, LV_PART_MAIN);
    y += 20;

    s_keyboard = lv_keyboard_create(scr);
    lv_obj_set_size(s_keyboard, ROUND_SCREEN_SIZE, 110);
    lv_obj_align(s_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(s_keyboard, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);

    if (s_textarea_pwd) {
        lv_keyboard_set_textarea(s_keyboard, s_textarea_pwd);
    }

    lv_obj_add_event_cb(s_keyboard, kb_ready_cb, LV_EVENT_READY, NULL);

    s_btn_connect = lv_button_create(scr);
    lv_obj_set_size(s_btn_connect, ROUND_SAFE_W, 36);
    lv_obj_align(s_btn_connect, LV_ALIGN_BOTTOM_MID, 0, -ROUND_SAFE_MARGIN);
    lv_obj_set_style_radius(s_btn_connect, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_btn_connect, lv_color_hex(0x3264C8), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_btn_connect, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_btn_connect, 0, LV_PART_MAIN);

    lv_obj_t *btn_label = lv_label_create(s_btn_connect);
    lv_obj_set_align(btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(btn_label, "Connect");
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn_label, lv_color_white(), LV_PART_MAIN);

    lv_obj_add_event_cb(s_btn_connect, connect_clicked_cb, LV_EVENT_CLICKED, NULL);

    ui_add_gesture_back(scr);

    ESP_LOGI(TAG, "wifi connect page created for %s (open=%d)",
             s_target_ssid, s_target_is_open);
}

static void page_wifi_conn_destroy(void)
{
    if (s_conn_timer) {
        lv_timer_del(s_conn_timer);
        s_conn_timer = NULL;
    }
    s_textarea_pwd = NULL;
    s_keyboard = NULL;
    s_btn_connect = NULL;
    s_label_status = NULL;
    s_page_wifi_conn.screen = NULL;
    ESP_LOGI(TAG, "wifi connect page destroyed");
}

static void page_wifi_conn_update(void)
{
}

ui_page_t *page_wifi_connect_get(void)
{
    if (!s_page_wifi_conn.create) {
        s_page_wifi_conn.create  = page_wifi_conn_create;
        s_page_wifi_conn.destroy = page_wifi_conn_destroy;
        s_page_wifi_conn.update  = page_wifi_conn_update;
    }
    return &s_page_wifi_conn;
}

void page_wifi_connect_set_target(const char *ssid, bool is_open)
{
    strncpy(s_target_ssid, ssid, 32);
    s_target_ssid[32] = '\0';
    s_target_is_open = is_open;
}