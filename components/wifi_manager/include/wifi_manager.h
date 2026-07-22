#pragma once

#include "esp_wifi.h"
#include <stdint.h>
#include <stdbool.h>

#define WIFI_SCAN_MAX 20

typedef struct {
    char ssid[33];
    int8_t rssi;
    wifi_auth_mode_t authmode;
    bool connected;
} wifi_ap_info_t;

typedef enum {
    WIFI_MGR_STATE_IDLE,
    WIFI_MGR_STATE_SCANNING,
    WIFI_MGR_STATE_CONNECTING,
    WIFI_MGR_STATE_CONNECTED,
    WIFI_MGR_STATE_DISCONNECTED,
} wifi_mgr_state_t;

typedef void (*wifi_mgr_scan_cb_t)(void);

typedef enum {
    WIFI_MGR_CONN_SUCCESS,
    WIFI_MGR_CONN_FAILED,
} wifi_mgr_conn_result_t;

typedef void (*wifi_mgr_conn_cb_t)(wifi_mgr_conn_result_t result);

void wifi_mgr_init(void);

wifi_mgr_state_t wifi_mgr_get_state(void);

void wifi_mgr_start_scan(void);

uint16_t wifi_mgr_get_ap_count(void);

const wifi_ap_info_t *wifi_mgr_get_ap_list(void);

bool wifi_mgr_is_connected(void);

void wifi_mgr_set_scan_done_cb(wifi_mgr_scan_cb_t cb);

void wifi_mgr_connect(const char *ssid, const char *password,
                       wifi_mgr_conn_cb_t cb);

void wifi_mgr_disconnect(void);

const char *wifi_mgr_get_connected_ssid(void);

void wifi_mgr_set_power_save(bool enable);