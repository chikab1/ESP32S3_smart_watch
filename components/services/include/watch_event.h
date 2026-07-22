#pragma once

#include "esp_event.h"

typedef enum {
    WATCH_EVENT_WIFI_CONNECTED,
    WATCH_EVENT_WIFI_DISCONNECTED,
    WATCH_EVENT_BATTERY_LOW,
    WATCH_EVENT_BATTERY_CHARGING,
    WATCH_EVENT_BATTERY_FULL,
    WATCH_EVENT_SCREEN_ON,
    WATCH_EVENT_SCREEN_OFF,
    WATCH_EVENT_POWER_DIM,
    WATCH_EVENT_POWER_SCREEN_OFF,
    WATCH_EVENT_POWER_PREPARE_SLEEP,
    WATCH_EVENT_POWER_WAKEUP,
    WATCH_EVENT_NOTIFICATION,
    WATCH_EVENT_STEP_UPDATE,
    WATCH_EVENT_TIME_SYNCED,
} watch_event_t;

esp_err_t watch_event_init(void);

esp_err_t watch_event_publish(watch_event_t event, void *data, size_t data_len);

esp_err_t watch_event_subscribe(watch_event_t event, esp_event_handler_t handler, void *arg);