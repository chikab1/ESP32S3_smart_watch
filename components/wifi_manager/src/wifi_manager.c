#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "watch_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "wifi_mgr";

#define SCAN_DONE_BIT   BIT0

static wifi_mgr_state_t s_state = WIFI_MGR_STATE_DISCONNECTED;
static wifi_ap_info_t s_ap_list[WIFI_SCAN_MAX];
static uint16_t s_ap_count = 0;
static EventGroupHandle_t s_event_group;
static wifi_mgr_scan_cb_t s_scan_done_cb;
static wifi_mgr_conn_cb_t s_conn_cb;
static bool s_initialized = false;
static char s_connecting_ssid[33];
static bool s_was_connected_before_scan = false;
static bool s_sntp_started = false;

static void sntp_sync_time_cb(struct timeval *tv)
{
    time_t now = tv->tv_sec;
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    ESP_LOGI(TAG, "SNTP sync OK: %04d-%02d-%02d %02d:%02d:%02d",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
             tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);
}

static void start_sntp(void)
{
    if (s_sntp_started) return;

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_setservername(1, "pool.ntp.org");
    esp_sntp_setservername(2, "time.nist.gov");
    esp_sntp_set_time_sync_notification_cb(sntp_sync_time_cb);
    esp_sntp_init();
    s_sntp_started = true;

    ESP_LOGI(TAG, "SNTP started");
}

static void stop_sntp(void)
{
    if (!s_sntp_started) return;
    esp_sntp_stop();
    s_sntp_started = false;
    ESP_LOGI(TAG, "SNTP stopped");
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA started");
            break;

        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t *evt = (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGI(TAG, "disconnected, reason=%d", evt->reason);
            if (s_state == WIFI_MGR_STATE_CONNECTING) {
                s_state = WIFI_MGR_STATE_DISCONNECTED;
                if (s_conn_cb) {
                    s_conn_cb(WIFI_MGR_CONN_FAILED);
                    s_conn_cb = NULL;
                }
            } else {
                s_state = WIFI_MGR_STATE_DISCONNECTED;
            }
            watch_event_publish(WATCH_EVENT_WIFI_DISCONNECTED, NULL, 0);
            break;
        }

        case WIFI_EVENT_SCAN_DONE: {
            wifi_event_sta_scan_done_t *evt = (wifi_event_sta_scan_done_t *)event_data;
            ESP_LOGI(TAG, "scan done, status=%u, num=%u", evt->status, evt->number);

            s_ap_count = 0;
            memset(s_ap_list, 0, sizeof(s_ap_list));

            if (evt->status == 0 && evt->number > 0) {
                uint16_t num = evt->number;
                if (num > WIFI_SCAN_MAX) num = WIFI_SCAN_MAX;

                wifi_ap_record_t records[WIFI_SCAN_MAX];
                uint16_t actual = num;
                esp_wifi_scan_get_ap_records(&actual, records);

                for (uint16_t i = 0; i < actual; i++) {
                    strncpy(s_ap_list[i].ssid, (char *)records[i].ssid, 32);
                    s_ap_list[i].ssid[32] = '\0';
                    s_ap_list[i].rssi = records[i].rssi;
                    s_ap_list[i].authmode = records[i].authmode;
                    s_ap_list[i].connected = false;
                }
                s_ap_count = actual;
            }

            if (s_was_connected_before_scan) {
                s_state = WIFI_MGR_STATE_CONNECTED;
                for (uint16_t i = 0; i < s_ap_count; i++) {
                    if (strcmp(s_ap_list[i].ssid, s_connecting_ssid) == 0) {
                        s_ap_list[i].connected = true;
                    }
                }
            } else {
                s_state = WIFI_MGR_STATE_DISCONNECTED;
            }

            xEventGroupSetBits(s_event_group, SCAN_DONE_BIT);

            if (s_scan_done_cb) {
                s_scan_done_cb();
            }
            break;
        }

        default:
            break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *evt = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
            s_state = WIFI_MGR_STATE_CONNECTED;

            for (uint16_t i = 0; i < s_ap_count; i++) {
                if (strcmp(s_ap_list[i].ssid, s_connecting_ssid) == 0) {
                    s_ap_list[i].connected = true;
                } else {
                    s_ap_list[i].connected = false;
                }
            }

            start_sntp();

            watch_event_publish(WATCH_EVENT_WIFI_CONNECTED, NULL, 0);

            if (s_conn_cb) {
                s_conn_cb(WIFI_MGR_CONN_SUCCESS);
                s_conn_cb = NULL;
            }
        }
    }
}

void wifi_mgr_init(void)
{
    if (s_initialized) return;

    setenv("TZ", "CST-8", 1);
    tzset();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erase, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (esp_netif_init() != ESP_OK) {
        ESP_LOGW(TAG, "netif already initialized");
    }

    if (esp_event_loop_create_default() != ESP_OK) {
        ESP_LOGW(TAG, "event loop already created");
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
        ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
        IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t existing_cfg = { 0 };
    esp_wifi_get_config(WIFI_IF_STA, &existing_cfg);

    if (existing_cfg.sta.ssid[0] != '\0') {
        ESP_LOGI(TAG, "found saved WiFi: %s, auto-connecting...", existing_cfg.sta.ssid);
        strncpy(s_connecting_ssid, (char *)existing_cfg.sta.ssid, 32);
        s_connecting_ssid[32] = '\0';
        s_state = WIFI_MGR_STATE_CONNECTING;
    } else {
        wifi_config_t wifi_config = { 0 };
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        s_state = WIFI_MGR_STATE_DISCONNECTED;
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    s_event_group = xEventGroupCreate();
    s_initialized = true;

    ESP_LOGI(TAG, "WiFi manager initialized");
}

wifi_mgr_state_t wifi_mgr_get_state(void)
{
    return s_state;
}

void wifi_mgr_start_scan(void)
{
    if (!s_initialized) return;

    s_was_connected_before_scan = (s_state == WIFI_MGR_STATE_CONNECTED);
    s_state = WIFI_MGR_STATE_SCANNING;
    s_ap_count = 0;

    wifi_scan_config_t scan_conf = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
    };

    esp_err_t ret = esp_wifi_scan_start(&scan_conf, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "scan start failed: 0x%x", ret);
        if (s_was_connected_before_scan) {
            s_state = WIFI_MGR_STATE_CONNECTED;
        } else {
            s_state = WIFI_MGR_STATE_DISCONNECTED;
        }
    } else {
        ESP_LOGI(TAG, "scan started");
    }
}

uint16_t wifi_mgr_get_ap_count(void)
{
    return s_ap_count;
}

const wifi_ap_info_t *wifi_mgr_get_ap_list(void)
{
    return s_ap_list;
}

bool wifi_mgr_is_connected(void)
{
    return s_state == WIFI_MGR_STATE_CONNECTED;
}

void wifi_mgr_set_scan_done_cb(wifi_mgr_scan_cb_t cb)
{
    s_scan_done_cb = cb;
}

const char *wifi_mgr_get_connected_ssid(void)
{
    if (s_state == WIFI_MGR_STATE_CONNECTED && s_connecting_ssid[0] != '\0') {
        return s_connecting_ssid;
    }
    return NULL;
}

void wifi_mgr_connect(const char *ssid, const char *password,
                       wifi_mgr_conn_cb_t cb)
{
    if (!s_initialized) return;

    s_conn_cb = cb;
    s_state = WIFI_MGR_STATE_CONNECTING;
    strncpy(s_connecting_ssid, ssid, 32);
    s_connecting_ssid[32] = '\0';

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid, ssid, 32);
    wifi_config.sta.ssid[32] = '\0';
    if (password) {
        strncpy((char *)wifi_config.sta.password, password, 63);
        wifi_config.sta.password[63] = '\0';
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_connect();

    ESP_LOGI(TAG, "connecting to %s...", ssid);
}

void wifi_mgr_disconnect(void)
{
    if (!s_initialized) return;
    esp_wifi_disconnect();
    s_state = WIFI_MGR_STATE_DISCONNECTED;
    s_connecting_ssid[0] = '\0';
    stop_sntp();
}

void wifi_mgr_set_power_save(bool enable)
{
    if (!s_initialized) return;

    if (enable) {
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        ESP_LOGI(TAG, "WiFi power save ON (MIN_MODEM)");
    } else {
        esp_wifi_set_ps(WIFI_PS_NONE);
        ESP_LOGI(TAG, "WiFi power save OFF");
    }
}