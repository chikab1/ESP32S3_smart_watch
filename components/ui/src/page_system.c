#include "page_system.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "page_system";

static ui_page_t s_page_system = {
    .name    = "System",
    .screen  = NULL,
    .create  = NULL,
    .destroy = NULL,
    .update  = NULL,
};

static lv_obj_t *s_label_heap;
static lv_obj_t *s_label_tasks;
static lv_obj_t *s_label_uptime;

static lv_obj_t *create_info_card(lv_obj_t *parent, int32_t y,
                                  const char *title, const char *value,
                                  lv_color_t color)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(92), 52);
    lv_obj_set_pos(card, 0, y);
    lv_obj_set_align(card, LV_ALIGN_TOP_MID);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
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

static void page_system_create(void)
{
    s_page_system.screen = ui_create_screen();
    lv_obj_t *scr = s_page_system.screen;

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_pos(title, 10, 10);
    lv_label_set_text(title, LV_SYMBOL_LEFT " System");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);

    s_label_heap = create_info_card(scr, 45, "Free Heap",
                                    "-",
                                    lv_color_hex(0xDC80E6));
    s_label_tasks = create_info_card(scr, 105, "Tasks",
                                     "-",
                                     lv_color_hex(0xDC80E6));
    s_label_uptime = create_info_card(scr, 165, "Uptime",
                                      "-",
                                      lv_color_hex(0xDC80E6));

    ui_add_gesture_back(scr);

    ESP_LOGI(TAG, "system created");
}

static void page_system_destroy(void)
{
    s_label_heap = NULL;
    s_label_tasks = NULL;
    s_label_uptime = NULL;
    s_page_system.screen = NULL;
    ESP_LOGI(TAG, "system destroyed");
}

static void page_system_update(void)
{
}

ui_page_t *page_system_get(void)
{
    if (!s_page_system.create) {
        s_page_system.create  = page_system_create;
        s_page_system.destroy = page_system_destroy;
        s_page_system.update  = page_system_update;
    }
    return &s_page_system;
}