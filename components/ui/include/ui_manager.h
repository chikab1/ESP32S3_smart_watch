#pragma once

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

#define PAGE_STACK_DEPTH 8

typedef struct {
    const char *name;
    lv_obj_t *screen;
    void (*create)(void);
    void (*destroy)(void);
    void (*update)(void);
} ui_page_t;

void ui_init(void);
void ui_update(void);
void ui_page_push(ui_page_t *page);
void ui_page_back(void);
ui_page_t *ui_page_current(void);