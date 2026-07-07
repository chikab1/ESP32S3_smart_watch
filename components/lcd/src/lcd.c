#include "lcd.h"
#include "gc9a01_reg.h"
#include "bsp_board.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

static spi_device_handle_t s_lcd;

static void lcd_write_cmd(uint8_t cmd)
{
    gpio_set_level(LCD_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(s_lcd, &t);
}

static void lcd_write_data(const void *data, size_t len)
{
    gpio_set_level(LCD_DC, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(s_lcd, &t);
}

static void lcd_set_window(uint16_t xs,
                           uint16_t ys,
                           uint16_t xe,
                           uint16_t ye)
{
    uint8_t buf[4];

    lcd_write_cmd(GC9A01_CASET);
    buf[0] = (xs >> 8) & 0xFF;
    buf[1] = xs & 0xFF;
    buf[2] = (xe >> 8) & 0xFF;
    buf[3] = xe & 0xFF;
    lcd_write_data(buf, 4);

    lcd_write_cmd(GC9A01_RASET);
    buf[0] = (ys >> 8) & 0xFF;
    buf[1] = ys & 0xFF;
    buf[2] = (ye >> 8) & 0xFF;
    buf[3] = ye & 0xFF;
    lcd_write_data(buf, 4);

    lcd_write_cmd(GC9A01_RAMWR);
}

esp_err_t lcd_init(void)
{
    gpio_reset_pin(LCD_DC);
    gpio_set_direction(LCD_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_DC, 1);

    gpio_reset_pin(LCD_RST);
    gpio_set_direction(LCD_RST, GPIO_MODE_OUTPUT);

    spi_bus_config_t bus_cfg = {
        .mosi_io_num   = LCD_MOSI,
        .miso_io_num   = LCD_MISO,
        .sclk_io_num   = LCD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * 40 * 2,
    };

    esp_err_t ret;
    spi_host_device_t host = SPI2_HOST;
    ret = spi_bus_initialize(host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        return ret;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode           = 0,
        .spics_io_num   = LCD_CS,
        .queue_size     = 1,
        .cs_ena_pretrans  = 1,
        .cs_ena_posttrans = 1,
    };

    ret = spi_bus_add_device(host, &dev_cfg, &s_lcd);
    if (ret != ESP_OK) {
        return ret;
    }

    gpio_set_level(LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    lcd_write_cmd(0xEF);
    lcd_write_cmd(0xEB);
    uint8_t eb14 = 0x14;
    lcd_write_data(&eb14, 1);

    lcd_write_cmd(0xFE);
    lcd_write_cmd(0xEF);

    lcd_write_cmd(0xEB);
    lcd_write_data(&eb14, 1);

    lcd_write_cmd(0x84);
    uint8_t v84 = 0x40;
    lcd_write_data(&v84, 1);

    lcd_write_cmd(0x85);
    uint8_t v85 = 0xFF;
    lcd_write_data(&v85, 1);

    lcd_write_cmd(0x86);
    uint8_t v86 = 0xFF;
    lcd_write_data(&v86, 1);

    lcd_write_cmd(0x87);
    uint8_t v87 = 0xFF;
    lcd_write_data(&v87, 1);

    lcd_write_cmd(0x88);
    uint8_t v88 = 0x0A;
    lcd_write_data(&v88, 1);

    lcd_write_cmd(0x89);
    uint8_t v89 = 0x21;
    lcd_write_data(&v89, 1);

    lcd_write_cmd(0x8A);
    uint8_t v8a = 0x00;
    lcd_write_data(&v8a, 1);

    lcd_write_cmd(0x8B);
    uint8_t v8b = 0x80;
    lcd_write_data(&v8b, 1);

    lcd_write_cmd(0x8C);
    uint8_t v8c = 0x01;
    lcd_write_data(&v8c, 1);

    lcd_write_cmd(0x8D);
    uint8_t v8d = 0x01;
    lcd_write_data(&v8d, 1);

    lcd_write_cmd(0x8E);
    uint8_t v8e = 0xFF;
    lcd_write_data(&v8e, 1);

    lcd_write_cmd(0x8F);
    uint8_t v8f = 0xFF;
    lcd_write_data(&v8f, 1);

    lcd_write_cmd(0xB6);
    uint8_t b6[] = {0x00, 0x20};
    lcd_write_data(b6, 2);

    lcd_write_cmd(0x36);
    uint8_t madctl = 0x08;
    lcd_write_data(&madctl, 1);

    lcd_write_cmd(0x3A);
    uint8_t colmod = 0x05;
    lcd_write_data(&colmod, 1);

    lcd_write_cmd(0x90);
    uint8_t v90[] = {0x08, 0x08, 0x08, 0x08};
    lcd_write_data(v90, 4);

    lcd_write_cmd(0xBD);
    uint8_t vbd = 0x06;
    lcd_write_data(&vbd, 1);

    lcd_write_cmd(0xBC);
    uint8_t vbc = 0x00;
    lcd_write_data(&vbc, 1);

    lcd_write_cmd(0xFF);
    uint8_t vff[] = {0x60, 0x01, 0x04};
    lcd_write_data(vff, 3);

    lcd_write_cmd(0xC3);
    uint8_t vc3 = 0x13;
    lcd_write_data(&vc3, 1);

    lcd_write_cmd(0xC4);
    uint8_t vc4 = 0x13;
    lcd_write_data(&vc4, 1);

    lcd_write_cmd(0xC9);
    uint8_t vc9 = 0x22;
    lcd_write_data(&vc9, 1);

    lcd_write_cmd(0xBE);
    uint8_t vbe = 0x11;
    lcd_write_data(&vbe, 1);

    lcd_write_cmd(0xE1);
    uint8_t ve1[] = {0x10, 0x0E};
    lcd_write_data(ve1, 2);

    lcd_write_cmd(0xDF);
    uint8_t vdf[] = {0x21, 0x0C, 0x02};
    lcd_write_data(vdf, 3);

    lcd_write_cmd(0xF0);
    uint8_t vf0[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
    lcd_write_data(vf0, 6);

    lcd_write_cmd(0xF1);
    uint8_t vf1[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
    lcd_write_data(vf1, 6);

    lcd_write_cmd(0xF2);
    uint8_t vf2[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
    lcd_write_data(vf2, 6);

    lcd_write_cmd(0xF3);
    uint8_t vf3[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
    lcd_write_data(vf3, 6);

    lcd_write_cmd(0xED);
    uint8_t ved[] = {0x1B, 0x0B};
    lcd_write_data(ved, 2);

    lcd_write_cmd(0xAE);
    uint8_t vae = 0x77;
    lcd_write_data(&vae, 1);

    lcd_write_cmd(0xCD);
    uint8_t vcd = 0x63;
    lcd_write_data(&vcd, 1);

    lcd_write_cmd(0x70);
    uint8_t v70[] = {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03};
    lcd_write_data(v70, 9);

    lcd_write_cmd(0xE8);
    uint8_t ve8 = 0x34;
    lcd_write_data(&ve8, 1);

    lcd_write_cmd(0x62);
    uint8_t v62[] = {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70,
                     0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70};
    lcd_write_data(v62, 12);

    lcd_write_cmd(0x63);
    uint8_t v63[] = {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70,
                     0x18, 0x13, 0x71, 0xF3, 0x70, 0x70};
    lcd_write_data(v63, 12);

    lcd_write_cmd(0x64);
    uint8_t v64[] = {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07};
    lcd_write_data(v64, 7);

    lcd_write_cmd(0x66);
    uint8_t v66[] = {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00};
    lcd_write_data(v66, 10);

    lcd_write_cmd(0x67);
    uint8_t v67[] = {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98};
    lcd_write_data(v67, 10);

    lcd_write_cmd(0x74);
    uint8_t v74[] = {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00};
    lcd_write_data(v74, 7);

    lcd_write_cmd(0x98);
    uint8_t v98[] = {0x3E, 0x07};
    lcd_write_data(v98, 2);

    lcd_write_cmd(0x35);
    lcd_write_cmd(0x21);

    lcd_write_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_write_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(20));

    return ESP_OK;
}

esp_err_t lcd_fill_color(uint16_t color)
{
    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    static uint16_t buf[LCD_WIDTH];
    for (int i = 0; i < LCD_WIDTH; i++) {
        buf[i] = color;
    }

    for (int row = 0; row < LCD_HEIGHT; row++) {
        lcd_write_data(buf, sizeof(buf));
    }

    return ESP_OK;
}

esp_err_t lcd_fill_rect(uint16_t x,
                        uint16_t y,
                        uint16_t w,
                        uint16_t h,
                        uint16_t color)
{
    lcd_set_window(x, y, x + w - 1, y + h - 1);

    static uint16_t buf[LCD_WIDTH];
    for (int i = 0; i < w; i++) {
        buf[i] = color;
    }

    for (int row = 0; row < h; row++) {
        lcd_write_data(buf, w * sizeof(uint16_t));
    }

    return ESP_OK;
}

esp_err_t lcd_draw_pixel(uint16_t x,
                         uint16_t y,
                         uint16_t color)
{
    lcd_set_window(x, y, x, y);
    lcd_write_data(&color, sizeof(color));
    return ESP_OK;
}

esp_err_t lcd_draw_bitmap(uint16_t x,
                          uint16_t y,
                          uint16_t w,
                          uint16_t h,
                          const void *data)
{
    lcd_set_window(x, y, x + w - 1, y + h - 1);

    size_t row_bytes = (size_t)w * 2;
    const uint8_t *ptr = (const uint8_t *)data;

    for (int row = 0; row < h; row++) {
        lcd_write_data(ptr, row_bytes);
        ptr += row_bytes;
    }

    return ESP_OK;
}