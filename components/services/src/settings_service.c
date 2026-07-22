#include "settings_service.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "settings";
static const char *NVS_NS = "watch_cfg";

#define KEY_BRIGHTNESS  "bright"
#define KEY_AUTO_SLEEP  "sleep"
#define KEY_WRIST_WAKE  "wake"
#define KEY_24H_FMT     "fmt24"

#define DEF_BRIGHTNESS  80
#define DEF_AUTO_SLEEP  15
#define DEF_WRIST_WAKE  true
#define DEF_24H_FMT     true

static nvs_handle_t s_nvs;
static bool s_inited = false;

esp_err_t settings_init(void)
{
    if (s_inited) return ESP_OK;

    esp_err_t ret = nvs_open(NVS_NS, NVS_READWRITE, &s_nvs);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs open failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_inited = true;
    ESP_LOGI(TAG, "init ok");
    return ESP_OK;
}

uint8_t settings_get_brightness(void)
{
    uint8_t v = DEF_BRIGHTNESS;
    nvs_get_u8(s_nvs, KEY_BRIGHTNESS, &v);
    return v;
}

void settings_set_brightness(uint8_t val)
{
    if (val > 100) val = 100;
    nvs_set_u8(s_nvs, KEY_BRIGHTNESS, val);
    nvs_commit(s_nvs);
}

uint16_t settings_get_auto_sleep(void)
{
    uint16_t v = DEF_AUTO_SLEEP;
    nvs_get_u16(s_nvs, KEY_AUTO_SLEEP, &v);
    return v;
}

void settings_set_auto_sleep(uint16_t sec)
{
    if (sec < 5) sec = 5;
    if (sec > 120) sec = 120;
    nvs_set_u16(s_nvs, KEY_AUTO_SLEEP, sec);
    nvs_commit(s_nvs);
}

bool settings_get_wrist_wake(void)
{
    uint8_t v = DEF_WRIST_WAKE ? 1 : 0;
    nvs_get_u8(s_nvs, KEY_WRIST_WAKE, &v);
    return v != 0;
}

void settings_set_wrist_wake(bool en)
{
    nvs_set_u8(s_nvs, KEY_WRIST_WAKE, en ? 1 : 0);
    nvs_commit(s_nvs);
}

bool settings_get_24h_format(void)
{
    uint8_t v = DEF_24H_FMT ? 1 : 0;
    nvs_get_u8(s_nvs, KEY_24H_FMT, &v);
    return v != 0;
}

void settings_set_24h_format(bool en)
{
    nvs_set_u8(s_nvs, KEY_24H_FMT, en ? 1 : 0);
    nvs_commit(s_nvs);
}