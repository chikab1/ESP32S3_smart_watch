#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define BATTERY_ADC_UNIT    ADC_UNIT_1
#define BATTERY_ADC_CH      ADC_CHANNEL_0
#define BATTERY_ADC_ATTEN   ADC_ATTEN_DB_12
#define BATTERY_VOLTAGE_DIV 3.0f

esp_err_t battery_adc_init(void);

uint8_t battery_get_percent(void);

float battery_get_voltage(void);

bool battery_is_present(void);