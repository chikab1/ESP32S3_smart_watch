#include "app_task.h"
#include "power_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "qmi8658.h"
#include "bsp_i2c.h"
#include "esp_log.h"
#include <stdio.h>

static qmi8658_t s_imu;
static volatile bool s_imu_suspend = false;

qmi8658_t *imu_get_handle(void)
{
    return &s_imu;
}

void imu_suspend(void)
{
    s_imu_suspend = true;
}

void imu_resume(void)
{
    s_imu_suspend = false;
}

void imu_deinit(void)
{
    qmi8658_deinit(&s_imu);
}

void imu_init_after_wakeup(void)
{
    i2c_master_bus_handle_t bus = bsp_i2c_get_bus();

    qmi8658_config_t cfg = {
        .address    = QMI8658_I2C_ADDR,
        .acc_range  = QMI8658_ACC_RANGE_4G,
        .gyro_range = QMI8658_GYRO_RANGE_512DPS,
        .acc_odr    = QMI8658_ACC_ODR_125HZ,
        .gyro_odr   = QMI8658_GYRO_ODR_125HZ,
    };

    esp_err_t ret = qmi8658_init(&s_imu, bus, &cfg);
    if (ret != ESP_OK) {
        ESP_LOGE("imu", "reinit after wakeup failed: 0x%x", ret);
        return;
    }

    ESP_LOGI("imu", "reinit OK after wakeup");
}

void imu_task(void *arg)
{
    i2c_master_bus_handle_t bus = bsp_i2c_get_bus();

    qmi8658_config_t cfg = {
        .address    = QMI8658_I2C_ADDR,
        .acc_range  = QMI8658_ACC_RANGE_4G,
        .gyro_range = QMI8658_GYRO_RANGE_512DPS,
        .acc_odr    = QMI8658_ACC_ODR_125HZ,
        .gyro_odr   = QMI8658_GYRO_ODR_125HZ,
    };

    esp_err_t ret = qmi8658_init(&s_imu, bus, &cfg);
    if (ret != ESP_OK) {
        printf("IMU init failed: 0x%x\r\n", ret);
        vTaskDelete(NULL);
        return;
    }

    printf("IMU init OK\r\n");

    qmi8658_data_t data;
    power_state_t last_state = POWER_STATE_ACTIVE;

    while (1)
    {
        if (s_imu_suspend) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        power_state_t cur_state = power_service_get_state();

        if (cur_state != last_state) {
            if (cur_state == POWER_STATE_SCREEN_OFF) {
                qmi8658_set_odr(&s_imu, QMI8658_ACC_ODR_62_5HZ, QMI8658_GYRO_ODR_62_5HZ);
            } else {
                qmi8658_set_odr(&s_imu, QMI8658_ACC_ODR_125HZ, QMI8658_GYRO_ODR_125HZ);
            }
            last_state = cur_state;
        }

        if (qmi8658_read(&s_imu, &data) == ESP_OK) {
            xQueueOverwrite(imu_queue, &data);
            power_service_feed_imu(data.ax, data.ay, data.az,
                                   data.gx, data.gy, data.gz);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}