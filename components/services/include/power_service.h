#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

typedef enum {
    POWER_STATE_ACTIVE,
    POWER_STATE_DIM,
    POWER_STATE_SCREEN_OFF,
} power_state_t;

typedef struct {
    uint8_t  brightness;
    uint16_t screen_timeout_sec;
    bool     wrist_wake_enabled;
    bool     usb_powered;
} power_config_t;

esp_err_t power_service_init(void);

void power_service_set_brightness(uint8_t percent);

uint8_t power_service_get_brightness(void);

void power_service_set_screen_timeout(uint16_t seconds);

uint16_t power_service_get_screen_timeout(void);

void power_service_set_wrist_wake(bool enabled);

bool power_service_is_wrist_wake_enabled(void);

void power_service_screen_on(void);

void power_service_screen_off(void);

bool power_service_is_screen_on(void);

bool power_service_is_screen_off(void);

void power_service_reset_idle_timer(void);

power_state_t power_service_get_state(void);

power_config_t power_service_get_config(void);

void power_service_feed_imu(float ax, float ay, float az,
                            float gx, float gy, float gz);

void power_service_enter_light_sleep(void);

void power_service_wakeup(void);