#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

esp_err_t lvgl_port_init(i2c_master_bus_handle_t i2c_bus);

void lvgl_port_lock(void);
void lvgl_port_unlock(void);