#include "ui_manager.h"
#include "ui_style.h"
#include "page_home.h"
#include "page_menu.h"
#include "page_settings.h"
#include "page_imu.h"
#include "page_mqtt.h"
#include "page_about.h"
#include "page_system.h"
#include "status_bar.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *TAG = "ui_manager";

static ui_page_t *s_stack[PAGE_STACK_DEPTH];
static uint8_t s_top = 0;

static void stack_push(ui_page_t *page)
{
    if (s_top >= PAGE_STACK_DEPTH) return;
    s_stack[s_top++] = page;
}

static void stack_pop(void)
{
    if (s_top == 0) return;
    s_stack[--s_top] = NULL;
}

static ui_page_t *stack_top(void)
{
    if (s_top == 0) return NULL;
    return s_stack[s_top - 1];
}

ui_page_t *ui_page_current(void)
{
    return stack_top();
}

void ui_page_push(ui_page_t *page)
{
    if (!page) return;
    if (s_top >= PAGE_STACK_DEPTH - 1) {
        ESP_LOGE(TAG, "page stack full, cannot push %s", page->name);
        return;
    }

    ui_page_t *current = stack_top();
    if (current) {
        if (current->destroy) current->destroy();
        current->screen = NULL;
    }

    stack_push(page);
    page->create();

    ESP_LOGI(TAG, "push %s, heap=%u", page->name,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    if (page->screen) {
        lv_screen_load_anim(page->screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, true);
    }
}

void ui_page_back(void)
{
    if (s_top <= 1) {
        ESP_LOGW(TAG, "cannot go back, only home in stack");
        return;
    }

    ui_page_t *current = stack_top();
    if (current) {
        if (current->destroy) current->destroy();
        current->screen = NULL;
    }

    stack_pop();

    ui_page_t *prev = stack_top();
    if (prev) {
        prev->create();

        ESP_LOGI(TAG, "back to %s, heap=%u", prev->name,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

        if (prev->screen) {
            lv_screen_load_anim(prev->screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, true);
        }
    }
}

void ui_init(void)
{
    ui_style_init();

    status_bar_init();

    ui_page_t *home = page_home_get();
    stack_push(home);
    home->create();

    ESP_LOGI(TAG, "init: home screen=%p heap=%u",
             home->screen,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    if (home->screen) {
        lv_screen_load(home->screen);
    }
}

void ui_update(void)
{
    ui_page_t *current = stack_top();
    if (current && current->update) {
        current->update();
    }
}