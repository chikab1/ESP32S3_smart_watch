#include "ui_style.h"

static lv_color_t s_battery;
static lv_color_t s_date;
static lv_color_t s_step;
static lv_color_t s_temp;
static lv_color_t s_humi;
static lv_color_t s_hr;
static lv_color_t s_accent;
static lv_color_t s_title;
static lv_color_t s_dim;
static lv_color_t s_panel_bg;
static lv_color_t s_panel_pressed;

void ui_style_init(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(
        disp,
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        true,
        LV_FONT_DEFAULT
    );
    lv_disp_set_theme(disp, theme);

    s_battery       = lv_color_hex(0x19C819);
    s_date          = lv_color_hex(0xFF461E);
    s_step          = lv_color_hex(0x3278FF);
    s_temp          = lv_color_hex(0xF5A73A);
    s_humi          = lv_color_hex(0x14C8E1);
    s_hr            = lv_color_hex(0xE11432);
    s_accent        = lv_color_hex(0x3264C8);
    s_title         = lv_color_hex(0x1980E1);
    s_dim           = lv_color_hex(0x808080);
    s_panel_bg      = lv_color_hex(0x000000);
    s_panel_pressed = lv_color_hex(0x808080);
}

lv_color_t ui_color_battery(void)       { return s_battery; }
lv_color_t ui_color_date(void)          { return s_date; }
lv_color_t ui_color_step(void)          { return s_step; }
lv_color_t ui_color_temp(void)          { return s_temp; }
lv_color_t ui_color_humi(void)          { return s_humi; }
lv_color_t ui_color_hr(void)            { return s_hr; }
lv_color_t ui_color_accent(void)        { return s_accent; }
lv_color_t ui_color_title(void)         { return s_title; }
lv_color_t ui_color_dim(void)           { return s_dim; }
lv_color_t ui_color_panel_bg(void)      { return s_panel_bg; }
lv_color_t ui_color_panel_pressed(void) { return s_panel_pressed; }