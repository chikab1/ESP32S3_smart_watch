#include "watch_event.h"
#include "esp_log.h"

static const char *TAG = "watch_event";

ESP_EVENT_DEFINE_BASE(WATCH_EVENT_BASE);

static bool s_initialized = false;

esp_err_t watch_event_init(void)
{
    if (s_initialized) return ESP_OK;
    s_initialized = true;
    ESP_LOGI(TAG, "event bus initialized");
    return ESP_OK;
}

esp_err_t watch_event_publish(watch_event_t event, void *data, size_t data_len)
{
    return esp_event_post(WATCH_EVENT_BASE, event, data, data_len,
                          pdMS_TO_TICKS(100));
}

esp_err_t watch_event_subscribe(watch_event_t event, esp_event_handler_t handler, void *arg)
{
    return esp_event_handler_register(WATCH_EVENT_BASE, event, handler, arg);
}