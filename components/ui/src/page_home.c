#include "page_home.h"
#include "page_menu.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
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

static lv_obj_t *s_btn_test;
static lv_obj_t *s_label_status;
static int s_press_count = 0;

static void btn_pressed_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, ">>> BUTTON PRESSED! count=%d", ++s_press_count);
    lv_label_set_text(s_label_status, "PRESSED!");
}

static void btn_clicked_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, ">>> BUTTON CLICKED!");
    lv_label_set_text(s_label_status, "CLICKED!");
}

static void btn_long_pressed_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, ">>> BUTTON LONG PRESSED!");
    lv_label_set_text(s_label_status, "LONG PRESS!");
}

static void screen_pressed_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_current_target(e);
    ESP_LOGI(TAG, ">>> SCREEN PRESSED! target=%p screen=%p", target, s_page_home.screen);
}

static void screen_released_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, ">>> SCREEN RELEASED!");
}

static void screen_gesture_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    const char *dir_name = "?";
    switch (dir) {
        case LV_DIR_TOP:    dir_name = "TOP (up)";    break;
        case LV_DIR_BOTTOM: dir_name = "BOTTOM (down)"; break;
        case LV_DIR_LEFT:   dir_name = "LEFT";        break;
        case LV_DIR_RIGHT:  dir_name = "RIGHT";       break;
        default: break;
    }
    ESP_LOGI(TAG, ">>> SCREEN GESTURE! dir=%s (%d)", dir_name, dir);
    lv_label_set_text_fmt(s_label_status, "GESTURE: %s", dir_name);

    if (dir == LV_DIR_TOP) {
        ESP_LOGI(TAG, ">>> SWIPE UP -> push Menu");
        ui_page_push(page_menu_get());
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

    lv_obj_add_event_cb(scr, screen_pressed_cb,  LV_EVENT_PRESSED,  NULL);
    lv_obj_add_event_cb(scr, screen_released_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(scr, screen_gesture_cb,  LV_EVENT_GESTURE,  NULL);

    ESP_LOGI(TAG, "screen=%p CLICKABLE=%d",
             scr,
             lv_obj_has_flag(scr, LV_OBJ_FLAG_CLICKABLE));

    s_btn_test = lv_button_create(scr);
    lv_obj_set_size(s_btn_test, 120, 50);
    lv_obj_align(s_btn_test, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *btn_label = lv_label_create(s_btn_test);
    lv_label_set_text(btn_label, "TEST");
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(s_btn_test, btn_pressed_cb,       LV_EVENT_PRESSED,       NULL);
    lv_obj_add_event_cb(s_btn_test, btn_clicked_cb,       LV_EVENT_CLICKED,       NULL);
    lv_obj_add_event_cb(s_btn_test, btn_long_pressed_cb,  LV_EVENT_LONG_PRESSED,  NULL);

    ESP_LOGI(TAG, "button=%p CLICKABLE=%d",
             s_btn_test,
             lv_obj_has_flag(s_btn_test, LV_OBJ_FLAG_CLICKABLE));

    s_label_status = lv_label_create(scr);
    lv_label_set_text(s_label_status, "Swipe up or tap button");
    lv_obj_align(s_label_status, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_color(s_label_status, lv_color_white(), LV_PART_MAIN);

    lv_obj_t *label_hint = lv_label_create(scr);
    lv_label_set_text(label_hint, "LVGL Event Test");
    lv_obj_align(label_hint, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(label_hint, lv_color_hex(0x808080), LV_PART_MAIN);

    ESP_LOGI(TAG, "home created (MINIMAL TEST)");
}

static void page_home_destroy(void)
{
    s_btn_test = NULL;
    s_label_status = NULL;
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