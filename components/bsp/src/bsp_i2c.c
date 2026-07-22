#include "bsp_i2c.h"
#include "bsp_board.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "bsp_i2c";
static i2c_master_bus_handle_t s_i2c_bus = NULL;
static SemaphoreHandle_t s_i2c_mutex = NULL;

esp_err_t bsp_i2c_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = I2C_NUM_0,
        .sda_io_num        = BSP_I2C_SDA,
        .scl_io_num        = BSP_I2C_SCL,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
    if (ret == ESP_OK && !s_i2c_mutex) {
        s_i2c_mutex = xSemaphoreCreateMutex();
    }
    return ret;
}

esp_err_t bsp_i2c_reinit(void)
{
    if (s_i2c_bus) {
        esp_err_t ret = i2c_del_master_bus(s_i2c_bus);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "delete bus failed: %s", esp_err_to_name(ret));
            return ret;
        }
        s_i2c_bus = NULL;
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = I2C_NUM_0,
        .sda_io_num        = BSP_I2C_SDA,
        .scl_io_num        = BSP_I2C_SCL,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "I2C bus reinitialized");
    } else {
        ESP_LOGE(TAG, "I2C reinit failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

i2c_master_bus_handle_t bsp_i2c_get_bus(void)
{
    return s_i2c_bus;
}

void *bsp_i2c_get_mutex(void)
{
    return s_i2c_mutex;
}