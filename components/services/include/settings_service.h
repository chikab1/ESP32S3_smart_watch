#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

esp_err_t settings_init(void);

uint8_t  settings_get_brightness(void);
void     settings_set_brightness(uint8_t val);

uint16_t settings_get_auto_sleep(void);
void     settings_set_auto_sleep(uint16_t sec);

bool     settings_get_wrist_wake(void);
void     settings_set_wrist_wake(bool en);

bool     settings_get_24h_format(void);
void     settings_set_24h_format(bool en);