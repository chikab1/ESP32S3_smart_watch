#pragma once

#include "ui_manager.h"
#include "lvgl.h"
#include <stdint.h>

lv_obj_t *ui_create_screen(void);

lv_obj_t *ui_create_menu_panel(lv_obj_t *parent, int32_t y_offset);

lv_obj_t *ui_create_menu_icon(lv_obj_t *panel, lv_color_t color,
                               const char *symbol);

lv_obj_t *ui_create_menu_label(lv_obj_t *panel, const char *text);

void ui_setup_screen_base(lv_obj_t *screen);

void ui_add_gesture_back(lv_obj_t *screen);

void ui_add_gesture_push(lv_obj_t *screen, ui_page_t *target);