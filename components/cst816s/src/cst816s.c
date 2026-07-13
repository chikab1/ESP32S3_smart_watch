#include "cst816s.h"
#include "cst816s_reg.h"
#include "bsp_board.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static esp_err_t cst816s_write_reg(cst816s_t *tp,
                                   uint8_t reg,
                                   uint8_t data)
{
    uint8_t buf[2] = { reg, data };
    return i2c_master_transmit(tp->dev, buf, 2, 50);
}

static esp_err_t cst816s_read_reg(cst816s_t *tp,
                                  uint8_t reg,
                                  uint8_t *data)
{
    return i2c_master_transmit_receive(tp->dev, &reg, 1, data, 1, 50);
}

static esp_err_t cst816s_read_regs(cst816s_t *tp,
                                   uint8_t reg,
                                   uint8_t *buf,
                                   uint16_t len)
{
    return i2c_master_transmit_receive(tp->dev, &reg, 1, buf, len, 50);
}

esp_err_t cst816s_init(cst816s_t *tp,
                       i2c_master_bus_handle_t bus)
{
    memset(tp, 0, sizeof(cst816s_t));
    tp->address = CST816S_ADDR;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = CST816S_ADDR,
        .scl_speed_hz    = 400000,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &tp->dev);
    if (ret != ESP_OK) {
        return ret;
    }

    gpio_reset_pin(TP_RST);
    gpio_set_direction(TP_RST, GPIO_MODE_OUTPUT);

    gpio_set_level(TP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(300));

    uint8_t id = 0;
    ret = cst816s_read_id(tp, &id);
    if (ret != ESP_OK) {
        return ret;
    }

    if ((id != 0xB4) && (id != 0xB5)) {
        return ESP_ERR_NOT_FOUND;
    }

    ret = cst816s_write_reg(tp, CST816S_DIS_AUTO_SLEEP, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGW("cst816s", "disable auto-sleep failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI("cst816s", "auto-sleep disabled OK");
    }

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