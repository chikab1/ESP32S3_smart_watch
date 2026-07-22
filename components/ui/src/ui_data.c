#include "ui_data.h"
#include "battery.h"
#include "wifi_manager.h"
#include "app_task.h"
#include "qmi8658.h"
#include "power_service.h"
#include <time.h>
#include <sys/time.h>
#include <math.h>

static const char *s_weekday_names[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

static uint16_t s_step_count = 0;
static float s_last_acc_mag = 0.0f;
static bool s_step_peak = false;
static uint32_t s_step_cooldown = 0;

static qmi8658_data_t s_imu_cache = {0};
static bool s_imu_ready = false;

static uint16_t detect_step(float ax, float ay, float az)
{
    float mag = sqrtf(ax * ax + ay * ay + az * az);

    float delta = mag - s_last_acc_mag;
    s_last_acc_mag = mag;

    if (s_step_cooldown > 0) {
        s_step_cooldown--;
        return s_step_count;
    }

    if (!s_step_peak && delta > 0.3f) {
        s_step_peak = true;
    } else if (s_step_peak && delta < -0.3f) {
        s_step_peak = false;
        s_step_count++;
        s_step_cooldown = 8;
    }

    return s_step_count;
}

void ui_data_poll_imu(void)
{
    qmi8658_data_t data;
    if (imu_queue && xQueueReceive(imu_queue, &data, 0) == pdTRUE) {
        s_imu_cache = data;
        s_imu_ready = true;
        detect_step(data.ax, data.ay, data.az);
        power_service_feed_imu(data.ax, data.ay, data.az,
                               data.gx, data.gy, data.gz);
    }
}

ui_datetime_t ui_get_time(void)
{
    ui_datetime_t t = {0};

    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        time_t now = tv.tv_sec;
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        t.year    = tm_info.tm_year + 1900;
        t.month   = tm_info.tm_mon + 1;
        t.day     = tm_info.tm_mday;
        t.hour    = tm_info.tm_hour;
        t.minute  = tm_info.tm_min;
        t.second  = tm_info.tm_sec;
        t.weekday = tm_info.tm_wday;
    } else {
        t.year    = 2026;
        t.month   = 1;
        t.day     = 1;
        t.hour    = 0;
        t.minute  = 0;
        t.second  = 0;
        t.weekday = 0;
    }

    return t;
}

ui_sensor_data_t ui_get_sensor(void)
{
    ui_sensor_data_t s = {
        .battery_percent = battery_get_percent(),
        .battery_present = battery_is_present(),
        .step_count      = s_step_count,
        .temperature     = 25,
        .humidity        = 67,
        .heart_rate      = 72,
        .acc_x           = s_imu_cache.ax,
        .acc_y           = s_imu_cache.ay,
        .acc_z           = s_imu_cache.az,
        .gyro_x          = s_imu_cache.gx,
        .gyro_y          = s_imu_cache.gy,
        .gyro_z          = s_imu_cache.gz,
        .imu_ready       = s_imu_ready,
    };
    return s;
}

const char *ui_get_weekday_name(uint8_t weekday)
{
    if (weekday > 6) weekday = 0;
    return s_weekday_names[weekday];
}