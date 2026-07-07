#include "bsp_board.h"
#include "bsp_i2c.h"
#include "driver/gpio.h"

void bsp_init(void)
{
    gpio_reset_pin(LCD_BL);
    gpio_set_direction(LCD_BL, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_BL, 1);

    bsp_i2c_init();
}