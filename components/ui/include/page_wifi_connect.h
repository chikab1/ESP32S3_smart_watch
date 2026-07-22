#pragma once

#include "ui_manager.h"

ui_page_t *page_wifi_connect_get(void);

void page_wifi_connect_set_target(const char *ssid, bool is_open);