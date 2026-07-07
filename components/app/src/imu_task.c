#include "app_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "qmi8658.h"
#include "bsp_i2c.h"
#include <stdio.h>

static qmi8658_t s_imu;

void imu_task(void *arg)
{
    i2c_master_bus_handle_t bus = bsp_i2c_get_bus();

    qmi8658_config_t cfg = {
        .address    = QMI8658_I2C_ADDR,
        .acc_range  = QMI8658_ACC_RANGE_4G,
        .gyro_range = QMI8658_GYRO_RANGE_512DPS,
        .acc_odr    = QMI8658_ACC_ODR_1000HZ,
        .gyro_odr   = QMI8658_GYRO_ODR_1000HZ,
    };

    esp_err_t ret = qmi8658_init(&s_imu, bus, &cfg);
    if (ret != ESP_OK) {
        printf("IMU init failed: 0x%x\r\n", ret);
        vTaskDelete(NULL);
        return;
    }

    printf("IMU init OK\r\n");

    qmi8658_data_t data;
    while (1)
    {
        if (qmi8658_read(&s_imu, &data) == ESP_OK) {
            xQueueOverwrite(imu_queue, &data);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}