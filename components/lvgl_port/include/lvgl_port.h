#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include "cst816s.h"

esp_err_t lvgl_port_init(i2c_master_bus_handle_t i2c_bus);

void lvgl_port_lock(void);
void lvgl_port_unlock(void);

void lvgl_port_suspend(void);
void lvgl_port_resume(void);

cst816s_t *lvgl_port_get_touch(void);