#include "qmi8658.h"
#include "qmi8658_reg.h"
#include "bsp_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const float acc_scale_table[] = {
    2.0f / 32768.0f,
    4.0f / 32768.0f,
    8.0f / 32768.0f,
    16.0f / 32768.0f,
};

static const float gyro_scale_table[] = {
    16.0f / 32768.0f,
    32.0f / 32768.0f,
    64.0f / 32768.0f,
    128.0f / 32768.0f,
    256.0f / 32768.0f,
    512.0f / 32768.0f,
    1024.0f / 32768.0f,
};

static esp_err_t qmi8658_write_reg(qmi8658_t *imu,
                                   uint8_t reg,
                                   uint8_t data)
{
    void *mtx = bsp_i2c_get_mutex();
    if (mtx) xSemaphoreTake((SemaphoreHandle_t)mtx, portMAX_DELAY);

    uint8_t buf[2] = { reg, data };
    esp_err_t ret = i2c_master_transmit(imu->dev, buf, 2, -1);

    if (mtx) xSemaphoreGive((SemaphoreHandle_t)mtx);
    return ret;
}

static esp_err_t qmi8658_read_reg(qmi8658_t *imu,
                                  uint8_t reg,
                                  uint8_t *data)
{
    void *mtx = bsp_i2c_get_mutex();
    if (mtx) xSemaphoreTake((SemaphoreHandle_t)mtx, portMAX_DELAY);

    esp_err_t ret = i2c_master_transmit_receive(imu->dev, &reg, 1, data, 1, -1);

    if (mtx) xSemaphoreGive((SemaphoreHandle_t)mtx);
    return ret;
}

static esp_err_t qmi8658_read_regs(qmi8658_t *imu,
                                   uint8_t reg,
                                   uint8_t *buf,
                                   uint16_t len)
{
    void *mtx = bsp_i2c_get_mutex();
    if (mtx) xSemaphoreTake((SemaphoreHandle_t)mtx, portMAX_DELAY);

    esp_err_t ret = i2c_master_transmit_receive(imu->dev, &reg, 1, buf, len, -1);

    if (mtx) xSemaphoreGive((SemaphoreHandle_t)mtx);
    return ret;
}

esp_err_t qmi8658_init(qmi8658_t *imu,
                       i2c_master_bus_handle_t bus,
                       const qmi8658_config_t *cfg)
{
    memset(imu, 0, sizeof(qmi8658_t));
    imu->address = cfg->address;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = cfg->address,
        .scl_speed_hz    = 400000,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &imu->dev);
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t id = 0;
    ret = qmi8658_read_id(imu, &id);
    if (ret != ESP_OK) {
        return ret;
    }
    if (id != QMI8658_WHO_AM_I_VAL) {
        return ESP_ERR_NOT_FOUND;
    }

    if (cfg->acc_range > QMI8658_ACC_RANGE_16G) {
        return ESP_ERR_INVALID_ARG;
    }
    if (cfg->gyro_range > QMI8658_GYRO_RANGE_1024DPS) {
        return ESP_ERR_INVALID_ARG;
    }

    imu->acc_scale = acc_scale_table[cfg->acc_range];
    imu->gyro_scale = gyro_scale_table[cfg->gyro_range];

    ret = qmi8658_write_reg(imu, QMI8658_CTRL1, 0x60);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = qmi8658_write_reg(imu, QMI8658_CTRL2,
        (cfg->acc_range << QMI8658_CTRL2_ACC_RANGE_POS) |
        (cfg->acc_odr  << QMI8658_CTRL2_ACC_ODR_POS));
    if (ret != ESP_OK) {
        return ret;
    }

    ret = qmi8658_write_reg(imu, QMI8658_CTRL3,
        (cfg->gyro_range << QMI8658_CTRL3_GYRO_RANGE_POS) |
        (cfg->gyro_odr   << QMI8658_CTRL3_GYRO_ODR_POS));
    if (ret != ESP_OK) {
        return ret;
    }

    ret = qmi8658_write_reg(imu, QMI8658_CTRL7,
        (1 << QMI8658_CTRL7_ACC_EN_BIT) |
        (1 << QMI8658_CTRL7_GYRO_EN_BIT));
    if (ret != ESP_OK) {
        return ret;
    }

    imu->ready = true;
    return ESP_OK;
}

esp_err_t qmi8658_deinit(qmi8658_t *imu)
{
    if (!imu) return ESP_ERR_INVALID_ARG;

    if (imu->dev) {
        esp_err_t ret = i2c_master_bus_rm_device(imu->dev);
        if (ret != ESP_OK) {
            return ret;
        }
        imu->dev = NULL;
    }

    imu->ready = false;
    return ESP_OK;
}

esp_err_t qmi8658_read_id(qmi8658_t *imu,
                          uint8_t *id)
{
    return qmi8658_read_reg(imu, QMI8658_WHO_AM_I, id);
}

esp_err_t qmi8658_read_raw(qmi8658_t *imu,
                           int16_t acc[3],
                           int16_t gyro[3])
{
    uint8_t buf[12];
    esp_err_t ret = qmi8658_read_regs(imu, QMI8658_AX_L, buf, 12);
    if (ret != ESP_OK) {
        return ret;
    }

    acc[0] = (int16_t)(buf[1] << 8 | buf[0]);
    acc[1] = (int16_t)(buf[3] << 8 | buf[2]);
    acc[2] = (int16_t)(buf[5] << 8 | buf[4]);

    gyro[0] = (int16_t)(buf[7]  << 8 | buf[6]);
    gyro[1] = (int16_t)(buf[9]  << 8 | buf[8]);
    gyro[2] = (int16_t)(buf[11] << 8 | buf[10]);

    return ESP_OK;
}

esp_err_t qmi8658_read(qmi8658_t *imu,
                       qmi8658_data_t *data)
{
    int16_t acc[3];
    int16_t gyro[3];

    esp_err_t ret = qmi8658_read_raw(imu, acc, gyro);
    if (ret != ESP_OK) {
        return ret;
    }

    data->ax = acc[0]  * imu->acc_scale;
    data->ay = acc[1]  * imu->acc_scale;
    data->az = acc[2]  * imu->acc_scale;
    data->gx = gyro[0] * imu->gyro_scale;
    data->gy = gyro[1] * imu->gyro_scale;
    data->gz = gyro[2] * imu->gyro_scale;

    return ESP_OK;
}

esp_err_t qmi8658_set_odr(qmi8658_t *imu,
                          uint8_t acc_odr,
                          uint8_t gyro_odr)
{
    if (!imu->ready) return ESP_ERR_INVALID_STATE;

    uint8_t ctrl2 = 0, ctrl3 = 0;
    qmi8658_read_reg(imu, QMI8658_CTRL2, &ctrl2);
    qmi8658_read_reg(imu, QMI8658_CTRL3, &ctrl3);

    ctrl2 = (ctrl2 & ~(0x07 << QMI8658_CTRL2_ACC_ODR_POS)) |
            (acc_odr << QMI8658_CTRL2_ACC_ODR_POS);
    ctrl3 = (ctrl3 & ~(0x07 << QMI8658_CTRL3_GYRO_ODR_POS)) |
            (gyro_odr << QMI8658_CTRL3_GYRO_ODR_POS);

    esp_err_t ret = qmi8658_write_reg(imu, QMI8658_CTRL2, ctrl2);
    if (ret != ESP_OK) return ret;

    return qmi8658_write_reg(imu, QMI8658_CTRL3, ctrl3);
}