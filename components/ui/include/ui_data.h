#pragma once

#include <stdint.h>

typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  weekday;
} ui_datetime_t;

typedef struct {
    uint8_t battery_percent;
    uint16_t step_count;
    int8_t  temperature;
    uint8_t humidity;
    uint8_t heart_rate;
} ui_sensor_data_t;

ui_datetime_t    ui_get_time(void);
ui_sensor_data_t ui_get_sensor(void);