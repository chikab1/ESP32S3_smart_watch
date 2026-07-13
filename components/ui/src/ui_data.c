#include "ui_data.h"

ui_datetime_t ui_get_time(void)
{
    ui_datetime_t t = {
        .year    = 2026,
        .month   = 7,
        .day     = 12,
        .hour    = 12,
        .minute  = 0,
        .second  = 0,
        .weekday = 0,
    };
    return t;
}

ui_sensor_data_t ui_get_sensor(void)
{
    ui_sensor_data_t s = {
        .battery_percent = 70,
        .step_count      = 3256,
        .temperature     = 25,
        .humidity        = 67,
        .heart_rate      = 72,
    };
    return s;
}