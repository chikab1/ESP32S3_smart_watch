#pragma once

#include <stdint.h>
#include <stdbool.h>

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
    bool    battery_present;
    uint16_t step_count;
    int8_t  temperature;
    uint8_t humidity;
    uint8_t heart_rate;
    float   acc_x;
    float   acc_y;
    float   acc_z;
    float   gyro_x;
    float   gyro_y;
    float   gyro_z;
    bool    imu_ready;
} ui_sensor_data_t;

ui_datetime_t    ui_get_time(void);
ui_sensor_data_t ui_get_sensor(void);
const char      *ui_get_weekday_name(uint8_t weekday);
void             ui_data_poll_imu(void);