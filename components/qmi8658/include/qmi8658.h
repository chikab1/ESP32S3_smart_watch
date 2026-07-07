#ifndef __QMI8658_H__
#define __QMI8658_H__

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "qmi8658_reg.h"

typedef struct
{
    i2c_master_dev_handle_t dev;
    uint8_t address;
    float acc_scale;
    float gyro_scale;
    bool ready;

} qmi8658_t;

typedef struct
{
    uint8_t address;
    uint8_t acc_range;
    uint8_t gyro_range;
    uint16_t acc_odr;
    uint16_t gyro_odr;

} qmi8658_config_t;

typedef struct
{
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;

} qmi8658_data_t;

esp_err_t qmi8658_init(qmi8658_t *imu,
                       i2c_master_bus_handle_t bus,
                       const qmi8658_config_t *cfg);

esp_err_t qmi8658_read_id(qmi8658_t *imu,
                          uint8_t *id);

esp_err_t qmi8658_read_raw(qmi8658_t *imu,
                           int16_t acc[3],
                           int16_t gyro[3]);

esp_err_t qmi8658_read(qmi8658_t *imu,
                       qmi8658_data_t *data);

#endif