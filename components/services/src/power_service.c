#include "power_service.h"
#include "watch_event.h"
#include "settings_service.h"
#include "wifi_manager.h"
#include "lvgl_port.h"
#include "cst816s.h"
#include "lcd.h"
#include "bsp_i2c.h"
#include "bsp_board.h"
#include "battery.h"
#include "imu_manager.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "power_svc";

static power_config_t s_cfg = {
    .brightness         = 80,
    .screen_timeout_sec = 15,
    .wrist_wake_enabled = true,
    .usb_powered        = false,
};

static power_state_t s_state     = POWER_STATE_ACTIVE;
static bool          s_screen_on = true;
static uint32_t      s_idle_ticks = 0;
static bool          s_initialized = false;
static bool          s_sleeping  = false;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WRIST_PITCH_VIEW_MIN   -45.0f
#define WRIST_PITCH_VIEW_MAX    45.0f
#define WRIST_GYRO_THRESHOLD    40.0f
#define WRIST_MOTION_WINDOW     20
#define WRIST_COOLDOWN_US       3000000LL

#define LIGHT_SLEEP_DELAY_SEC   30

static bool     s_wrist_was_viewing = false;
static int64_t  s_wrist_last_wake_us = 0;
static int32_t  s_wrist_motion_countdown = 0;

#define BACKLIGHT_GPIO    LCD_BL
#define LEDC_SPEED        LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL      LEDC_CHANNEL_0
#define LEDC_TIMER        LEDC_TIMER_0
#define LEDC_RESOLUTION   LEDC_TIMER_10_BIT
#define LEDC_FREQ_HZ      5000

static void set_backlight_pwm(uint8_t percent)
{
    uint32_t duty = 0;
    if (percent > 0) {
        duty = (uint32_t)((1 << 10) - 1) * percent / 100;
    }
    ledc_set_duty(LEDC_SPEED, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_SPEED, LEDC_CHANNEL);
}

static void init_pwm(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_SPEED,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t ch_cfg = {
        .gpio_num   = BACKLIGHT_GPIO,
        .speed_mode = LEDC_SPEED,
        .channel    = LEDC_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ch_cfg);
}

static void configure_wakeup_gpios(void)
{
    gpio_config_t io_conf = {
        .intr_type    = GPIO_INTR_LOW_LEVEL,
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << TP_INT),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);
    gpio_wakeup_enable(TP_INT, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    ESP_LOGI(TAG, "wakeup GPIO configured (TP_INT=%d, LOW level)", TP_INT);
}

static void configure_wakeup_gpios(void);
static void power_service_task(void *arg);

esp_err_t power_service_init(void)
{
    if (s_initialized) return ESP_OK;

    s_cfg.brightness         = settings_get_brightness();
    s_cfg.screen_timeout_sec = settings_get_auto_sleep();
    s_cfg.wrist_wake_enabled = settings_get_wrist_wake();

    init_pwm();
    set_backlight_pwm(s_cfg.brightness);

    s_initialized = true;
    s_state = POWER_STATE_ACTIVE;
    s_screen_on = true;
    s_idle_ticks = 0;

    xTaskCreatePinnedToCore(power_service_task, "pwr_svc", 4096, NULL, 3, NULL, 1);

    ESP_LOGI(TAG, "initialized, brightness=%d%%, timeout=%ds",
             s_cfg.brightness, s_cfg.screen_timeout_sec);
    return ESP_OK;
}

void power_service_set_brightness(uint8_t percent)
{
    if (percent > 100) percent = 100;
    s_cfg.brightness = percent;
    settings_set_brightness(percent);
    if (s_screen_on) {
        set_backlight_pwm(percent);
    }
    ESP_LOGI(TAG, "brightness -> %d%%", percent);
}

uint8_t power_service_get_brightness(void)
{
    return s_cfg.brightness;
}

void power_service_set_screen_timeout(uint16_t seconds)
{
    s_cfg.screen_timeout_sec = seconds;
    settings_set_auto_sleep(seconds);
    ESP_LOGI(TAG, "screen timeout -> %ds", seconds);
}

uint16_t power_service_get_screen_timeout(void)
{
    return s_cfg.screen_timeout_sec;
}

void power_service_set_wrist_wake(bool enabled)
{
    s_cfg.wrist_wake_enabled = enabled;
    settings_set_wrist_wake(enabled);
    ESP_LOGI(TAG, "wrist wake -> %s", enabled ? "ON" : "OFF");
}

bool power_service_is_wrist_wake_enabled(void)
{
    return s_cfg.wrist_wake_enabled;
}

static void check_wrist_wake(float ax, float ay, float az,
                              float gx, float gy, float gz)
{
    if (!s_cfg.wrist_wake_enabled || s_screen_on) return;

    float pitch_deg = atan2f(ax, sqrtf(ay * ay + az * az))
                      * (180.0f / (float)M_PI);

    float gyro_mag = sqrtf(gx * gx + gy * gy + gz * gz);

    if (gyro_mag > WRIST_GYRO_THRESHOLD) {
        s_wrist_motion_countdown = WRIST_MOTION_WINDOW;
    }
    if (s_wrist_motion_countdown > 0) {
        s_wrist_motion_countdown--;
    }

    bool is_viewing = (pitch_deg >= WRIST_PITCH_VIEW_MIN &&
                       pitch_deg <= WRIST_PITCH_VIEW_MAX);

    if (!s_wrist_was_viewing && is_viewing && s_wrist_motion_countdown > 0) {
        int64_t now = esp_timer_get_time();
        if (now - s_wrist_last_wake_us > WRIST_COOLDOWN_US) {
            s_wrist_last_wake_us = now;
            power_service_wakeup();
            ESP_LOGI(TAG, "wrist wake triggered");
        }
    }

    s_wrist_was_viewing = is_viewing;
}

void power_service_feed_imu(float ax, float ay, float az,
                            float gx, float gy, float gz)
{
    check_wrist_wake(ax, ay, az, gx, gy, gz);
}

void power_service_screen_on(void)
{
    if (s_screen_on) return;
    s_screen_on = true;
    s_state = POWER_STATE_ACTIVE;
    s_idle_ticks = 0;
    set_backlight_pwm(s_cfg.brightness);
    watch_event_publish(WATCH_EVENT_SCREEN_ON, NULL, 0);
    ESP_LOGI(TAG, "screen ON");
}

void power_service_screen_off(void)
{
    if (!s_screen_on) return;
    s_screen_on = false;
    s_state = POWER_STATE_SCREEN_OFF;
    set_backlight_pwm(0);
    watch_event_publish(WATCH_EVENT_POWER_SCREEN_OFF, NULL, 0);
    ESP_LOGI(TAG, "screen OFF");
}

bool power_service_is_screen_on(void)
{
    return s_screen_on;
}

bool power_service_is_screen_off(void)
{
    return !s_screen_on;
}

void power_service_reset_idle_timer(void)
{
    s_idle_ticks = 0;
    if (s_state == POWER_STATE_DIM || s_state == POWER_STATE_SCREEN_OFF) {
        power_service_wakeup();
    }
}

power_state_t power_service_get_state(void)
{
    return s_state;
}

power_config_t power_service_get_config(void)
{
    return s_cfg;
}

void power_service_enter_light_sleep(void)
{
    if (s_sleeping) return;

    s_sleeping = true;

    ESP_LOGI(TAG, "TP_INT before sleep=%d", gpio_get_level(TP_INT));

    cst816s_t *tp = lvgl_port_get_touch();
    if (tp && tp->ready) {
        cst816s_clear_int(tp);
    }

    ESP_LOGI(TAG, "TP_INT after clear=%d", gpio_get_level(TP_INT));

    ESP_LOGI(TAG, "POWER ENTER LIGHT SLEEP");

    watch_event_publish(WATCH_EVENT_POWER_PREPARE_SLEEP, NULL, 0);

    lvgl_port_suspend();

    wifi_mgr_set_power_save(true);

    configure_wakeup_gpios();

    esp_light_sleep_start();

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_GPIO) {
        ESP_LOGI(TAG, "POWER WAKEUP SOURCE TOUCH");
    } else {
        ESP_LOGI(TAG, "wakeup cause=%d", cause);
    }

    power_service_wakeup();
}

void power_service_wakeup(void)
{
    if (!s_sleeping && s_screen_on) return;

    s_sleeping = false;
    s_screen_on = true;
    s_state = POWER_STATE_ACTIVE;
    s_idle_ticks = 0;

    ESP_LOGI(TAG, "POWER WAKEUP");

    /* Step 1: Suspend IMU task access to I2C */
    imu_suspend();

    /* Step 2: Remove all I2C devices from old bus */
    cst816s_t *tp = lvgl_port_get_touch();
    if (tp) {
        cst816s_deinit(tp);
    }

    imu_deinit();

    /* Step 3: Reinit I2C bus */
    esp_err_t ret = bsp_i2c_reinit();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C reinit FAILED");
    }

    /* Step 4: Resume CST816S (re-add I2C device) */
    if (tp) {
        i2c_master_bus_handle_t bus = bsp_i2c_get_bus();
        ret = cst816s_resume(tp, bus);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "CST816S RESUME OK");
        } else {
            ESP_LOGE(TAG, "CST816S RESUME FAILED: %s", esp_err_to_name(ret));
        }
    }

    /* Step 5: Reinit IMU */
    imu_init_after_wakeup();

    /* Step 6: Resume IMU task */
    imu_resume();

    /* Step 5: Reinit LCD (full init commands) */
    lcd_reinit();
    ESP_LOGI(TAG, "LCD RESUME OK");

    /* Step 6: Resume LVGL */
    lvgl_port_resume();
    ESP_LOGI(TAG, "LVGL RESUME OK");

    /* Step 7: Restore backlight */
    set_backlight_pwm(s_cfg.brightness);

    /* Step 8: Restore WiFi */
    wifi_mgr_set_power_save(false);

    watch_event_publish(WATCH_EVENT_POWER_WAKEUP, NULL, 0);
    ESP_LOGI(TAG, "wakeup, screen ON");
}

void power_service_task(void *arg)
{
    uint32_t screen_off_ticks = 0;

    while (1) {
        if (!s_initialized) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (s_sleeping) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (!s_screen_on) {
            screen_off_ticks++;
            if (screen_off_ticks >= LIGHT_SLEEP_DELAY_SEC) {
                screen_off_ticks = 0;
                power_service_enter_light_sleep();
                continue;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        s_idle_ticks++;

        if (s_state == POWER_STATE_ACTIVE &&
            s_idle_ticks >= s_cfg.screen_timeout_sec * 2) {
            s_state = POWER_STATE_DIM;
            set_backlight_pwm(s_cfg.brightness / 4);
            watch_event_publish(WATCH_EVENT_POWER_DIM, NULL, 0);
            ESP_LOGI(TAG, "POWER ACTIVE -> DIM");
        }

        if (s_state == POWER_STATE_DIM &&
            s_idle_ticks >= s_cfg.screen_timeout_sec * 3) {
            power_service_screen_off();
            screen_off_ticks = 0;
            ESP_LOGI(TAG, "POWER DIM -> SCREEN_OFF");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}