#include "ui_utils.h"
#include "ui_manager.h"
#include "ui_style.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "ui_utils";

lv_obj_t *ui_create_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    ui_setup_screen_base(screen);
    return screen;
}

lv_obj_t *ui_create_screen_scrollable(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    ui_setup_screen_base(screen);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(screen, LV_DIR_VER);
    return screen;
}

void ui_setup_screen_base(lv_obj_t *screen)
{
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
}

lv_obj_t *ui_create_menu_panel(lv_obj_t *parent, int32_t y_offset)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, ROUND_PANEL_W, ROUND_PANEL_H);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, y_offset);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(panel, ROUND_PANEL_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, ui_color_panel_pressed(), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(panel, 100, LV_PART_MAIN | LV_STATE_PRESSED);
    return panel;
}

lv_obj_t *ui_create_menu_icon(lv_obj_t *panel, lv_color_t color,
                               const char *symbol)
{
    lv_obj_t *btn = lv_button_create(panel);
    lv_obj_set_size(btn, 36, 36);
    lv_obj_set_align(btn, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_margin_left(btn, 10, LV_PART_MAIN);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);

    lv_obj_t *icon = lv_label_create(btn);
    lv_obj_set_align(icon, LV_ALIGN_CENTER);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_color(icon, lv_color_white(), LV_PART_MAIN);

    return btn;
}

lv_obj_t *ui_create_menu_label(lv_obj_t *panel, const char *text)
{
    lv_obj_t *label = lv_label_create(panel);
    lv_obj_set_align(label, LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(label, 55, 0);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, LV_PART_MAIN);
    return label;
}

lv_obj_t *ui_create_page_title(lv_obj_t *parent, const char *text)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_obj_set_pos(title, 0, ROUND_TOP_OFFSET);
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_label_set_text(title, text);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    return title;
}

lv_obj_t *ui_create_info_card(lv_obj_t *parent, int32_t y,
                               const char *title, const char *value,
                               lv_color_t color)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, ROUND_SAFE_W, 50);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(card, 12, LV_PART_MAIN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_title, color, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 0, 4);

    lv_obj_t *lbl_val = lv_label_create(card);
    lv_label_set_text(lbl_val, value);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_val, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(lbl_val, LV_ALIGN_BOTTOM_LEFT, 0, -4);

    return lbl_val;
}

static void gesture_push_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    ui_page_t *target = (ui_page_t *)lv_event_get_user_data(e);
    if (dir == LV_DIR_TOP && target) {
        ESP_LOGI(TAG, "GESTURE UP -> push %s", target->name);
        ui_page_push(target);
    }
}

static void gesture_back_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_RIGHT) {
        ESP_LOGI(TAG, "GESTURE RIGHT -> back");
        ui_page_back();
    }
}

void ui_add_gesture_push(lv_obj_t *screen, ui_page_t *target)
{
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, gesture_push_cb, LV_EVENT_GESTURE, target);
    ESP_LOGI(TAG, "gesture PUSH on screen %p -> %s", screen, target->name);
}

void ui_add_gesture_back(lv_obj_t *screen)
{
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, gesture_back_cb, LV_EVENT_GESTURE, NULL);
    ESP_LOGI(TAG, "gesture BACK on screen %p", screen);
}