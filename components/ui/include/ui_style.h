#pragma once

#include "lvgl.h"

#define ROUND_SCREEN_SIZE    240
#define ROUND_SAFE_MARGIN    18
#define ROUND_SAFE_W         (ROUND_SCREEN_SIZE - 2 * ROUND_SAFE_MARGIN)
#define ROUND_SAFE_H         (ROUND_SCREEN_SIZE - 2 * ROUND_SAFE_MARGIN)
#define ROUND_TOP_OFFSET     25
#define ROUND_PANEL_W        ROUND_SAFE_W
#define ROUND_PANEL_H        55
#define ROUND_PANEL_GAP      6
#define ROUND_PANEL_RADIUS   12

void ui_style_init(void);

lv_color_t ui_color_battery(void);
lv_color_t ui_color_date(void);
lv_color_t ui_color_step(void);
lv_color_t ui_color_temp(void);
lv_color_t ui_color_humi(void);
lv_color_t ui_color_hr(void);
lv_color_t ui_color_accent(void);
lv_color_t ui_color_title(void);
lv_color_t ui_color_dim(void);
lv_color_t ui_color_panel_bg(void);
lv_color_t ui_color_panel_pressed(void);