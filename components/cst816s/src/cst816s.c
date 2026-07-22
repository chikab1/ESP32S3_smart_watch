#include "cst816s.h"
#include "cst816s_reg.h"
#include "bsp_board.h"
#include "bsp_i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static esp_err_t cst816s_write_reg(cst816s_t *tp,
                                   uint8_t reg,
                                   uint8_t data)
{
    void *mtx = bsp_i2c_get_mutex();
    if (mtx) xSemaphoreTake((SemaphoreHandle_t)mtx, portMAX_DELAY);

    uint8_t buf[2] = { reg, data };
    esp_err_t ret = i2c_master_transmit(tp->dev, buf, 2, 50);

    if (mtx) xSemaphoreGive((SemaphoreHandle_t)mtx);
    return ret;
}

static esp_err_t cst816s_read_reg(cst816s_t *tp,
                                  uint8_t reg,
                                  uint8_t *data)
{
    void *mtx = bsp_i2c_get_mutex();
    if (mtx) xSemaphoreTake((SemaphoreHandle_t)mtx, portMAX_DELAY);

    esp_err_t ret = i2c_master_transmit_receive(tp->dev, &reg, 1, data, 1, 50);

    if (mtx) xSemaphoreGive((SemaphoreHandle_t)mtx);
    return ret;
}

static esp_err_t cst816s_read_regs(cst816s_t *tp,
                                   uint8_t reg,
                                   uint8_t *buf,
                                   uint16_t len)
{
    void *mtx = bsp_i2c_get_mutex();
    if (mtx) xSemaphoreTake((SemaphoreHandle_t)mtx, portMAX_DELAY);

    esp_err_t ret = i2c_master_transmit_receive(tp->dev, &reg, 1, buf, len, 50);

    if (mtx) xSemaphoreGive((SemaphoreHandle_t)mtx);
    return ret;
}

esp_err_t cst816s_init(cst816s_t *tp,
                       i2c_master_bus_handle_t bus)
{
    memset(tp, 0, sizeof(cst816s_t));
    tp->address = CST816S_ADDR;

    /* Initialize TP_INT GPIO as input with pull-up */
    gpio_config_t int_conf = {
        .pin_bit_mask = (1ULL << TP_INT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&int_conf);
    if (ret != ESP_OK) {
        ESP_LOGE("cst816s", "TP_INT gpio init failed: %s", esp_err_to_name(ret));
    }

    /* Register I2C device */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = CST816S_ADDR,
        .scl_speed_hz    = 400000,
    };

    ret = i2c_master_bus_add_device(bus, &dev_cfg, &tp->dev);
    if (ret != ESP_OK) {
        return ret;
    }

    /* Reset CST816S */
    gpio_reset_pin(TP_RST);
    gpio_set_direction(TP_RST, GPIO_MODE_OUTPUT);

    gpio_set_level(TP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(TP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    /* Read chip ID */
    uint8_t id = 0;
    ret = cst816s_read_id(tp, &id);
    if (ret != ESP_OK) {
        ESP_LOGE("cst816s", "read ID failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if ((id != 0xB4) && (id != 0xB5)) {
        ESP_LOGE("cst816s", "unknown chip ID=0x%02X", id);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI("cst816s", "chip ID=0x%02X", id);

    tp->ready = true;
    return ESP_OK;
}

esp_err_t cst816s_read_id(cst816s_t *tp,
                          uint8_t *id)
{
    return cst816s_read_reg(tp, CST816S_CHIP_ID, id);
}

esp_err_t cst816s_read(cst816s_t *tp,
                       cst816s_point_t *point)
{
    uint8_t finger_num = 0;

    esp_err_t ret = cst816s_read_reg(tp, CST816S_FINGER_NUM, &finger_num);
    if (ret != ESP_OK) {
        return ret;
    }

    if (finger_num == 0) {
        point->pressed = false;
        point->x = 0;
        point->y = 0;
        return ESP_OK;
    }

    uint8_t buf[4];
    ret = cst816s_read_regs(tp, CST816S_XPOS_H, buf, 4);
    if (ret != ESP_OK) {
        return ret;
    }

    point->pressed = true;
    point->x = ((buf[0] & 0x0F) << 8) | buf[1];
    point->y = ((buf[2] & 0x0F) << 8) | buf[3];

    return ESP_OK;
}

esp_err_t cst816s_clear_int(cst816s_t *tp)
{
    if (!tp || !tp->ready) return ESP_ERR_INVALID_STATE;

    uint8_t buf[6];
    return cst816s_read_regs(tp, CST816S_GESTURE_ID, buf, 6);
}

esp_err_t cst816s_deinit(cst816s_t *tp)
{
    if (!tp) return ESP_ERR_INVALID_ARG;

    if (tp->dev) {
        esp_err_t ret = i2c_master_bus_rm_device(tp->dev);
        if (ret != ESP_OK) {
            ESP_LOGE("cst816s", "remove device failed: %s", esp_err_to_name(ret));
            return ret;
        }
        tp->dev = NULL;
    }

    tp->ready = false;
    ESP_LOGI("cst816s", "device removed");
    return ESP_OK;
}

esp_err_t cst816s_resume(cst816s_t *tp,
                         i2c_master_bus_handle_t bus)
{
    if (!tp) return ESP_ERR_INVALID_ARG;

    /* Reset CST816S */
    gpio_set_level(TP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(TP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    /* Re-add I2C device to get fresh handle */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = CST816S_ADDR,
        .scl_speed_hz    = 400000,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &tp->dev);
    if (ret != ESP_OK) {
        ESP_LOGE("cst816s", "resume: add device failed: %s", esp_err_to_name(ret));
        tp->dev = NULL;
        tp->ready = false;
        return ret;
    }

    /* Read chip ID to verify communication */
    uint8_t id = 0;
    ret = cst816s_read_id(tp, &id);
    if (ret != ESP_OK) {
        ESP_LOGE("cst816s", "resume: read ID failed: %s", esp_err_to_name(ret));
        i2c_master_bus_rm_device(tp->dev);
        tp->dev = NULL;
        tp->ready = false;
        return ret;
    }

    if ((id != 0xB4) && (id != 0xB5)) {
        ESP_LOGE("cst816s", "resume: unknown chip ID=0x%02X", id);
        i2c_master_bus_rm_device(tp->dev);
        tp->dev = NULL;
        tp->ready = false;
        return ESP_ERR_NOT_FOUND;
    }

    /* Clear any pending interrupt */
    cst816s_clear_int(tp);

    tp->ready = true;
    ESP_LOGI("cst816s", "resume OK, chip ID=0x%02X", id);
    return ESP_OK;
}