#pragma once

#include <stdbool.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

typedef struct
{
    i2c_master_dev_handle_t dev;
    uint8_t address;
    bool ready;

} cst816s_t;

typedef struct
{
    bool pressed;
    uint16_t x;
    uint16_t y;

} cst816s_point_t;

esp_err_t cst816s_init(cst816s_t *tp,
                       i2c_master_bus_handle_t bus);

esp_err_t cst816s_read_id(cst816s_t *tp,
                          uint8_t *id);

esp_err_t cst816s_read(cst816s_t *tp,
                       cst816s_point_t *point);