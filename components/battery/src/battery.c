#include "battery.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "battery";

static adc_oneshot_unit_handle_t s_adc_unit;
static adc_cali_handle_t s_cali = NULL;
static bool s_initialized = false;

static bool cali_adc(void)
{
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = BATTERY_ADC_UNIT,
        .chan     = BATTERY_ADC_CH,
        .atten    = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "calibration failed, using raw ADC");
        return false;
    }
    return true;
}

esp_err_t battery_adc_init(void)
{
    if (s_initialized) return ESP_OK;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = BATTERY_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc_unit));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_unit, BATTERY_ADC_CH, &chan_cfg));

    cali_adc();

    s_initialized = true;
    ESP_LOGI(TAG, "ADC init OK (GPIO1/ADC1_CH0)");
    return ESP_OK;
}

static int read_adc_mv(void)
{
    int raw = 0;
    esp_err_t ret = adc_oneshot_read(s_adc_unit, BATTERY_ADC_CH, &raw);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC read failed");
        return 0;
    }

    if (s_cali) {
        int mv = 0;
        adc_cali_raw_to_voltage(s_cali, raw, &mv);
        return mv;
    }

    return (int)((float)raw / 4095.0f * 3300.0f);
}

static float s_last_voltage = 0.0f;
static bool s_battery_present = false;

float battery_get_voltage(void)
{
    if (!s_initialized) return 0.0f;

    int readings[16];
    const int samples = 16;
    int sum = 0;
    for (int i = 0; i < samples; i++) {
        readings[i] = read_adc_mv();
        sum += readings[i];
    }
    int avg_mv = sum / samples;

    int variance = 0;
    for (int i = 0; i < samples; i++) {
        int diff = readings[i] - avg_mv;
        variance += diff * diff;
    }
    variance /= samples;

    float v_bat = (float)avg_mv / 1000.0f * BATTERY_VOLTAGE_DIV;

    s_battery_present = true;
    if (v_bat < 2.8f || v_bat > 4.35f) {
        s_battery_present = false;
    }
    if (variance > 40000) {
        s_battery_present = false;
    }

    if (!s_battery_present) {
        v_bat = 0.0f;
    }

    s_last_voltage = v_bat;
    return v_bat;
}

bool battery_is_present(void)
{
    return s_battery_present;
}

uint8_t battery_get_percent(void)
{
    float v = battery_get_voltage();

    if (!s_battery_present) return 0;

    if (v >= 4.20f) return 100;
    if (v >= 4.10f) return 95;
    if (v >= 4.00f) return 85;
    if (v >= 3.90f) return 75;
    if (v >= 3.80f) return 60;
    if (v >= 3.70f) return 45;
    if (v >= 3.60f) return 30;
    if (v >= 3.50f) return 15;
    if (v >= 3.40f) return 5;
    return 0;
}