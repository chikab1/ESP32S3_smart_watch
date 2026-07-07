#pragma once

#define BSP_I2C_SDA            6
#define BSP_I2C_SCL            7

#define QMI_INT1               4
#define QMI_INT2               3

#define TP_INT                 5
#define TP_RST                 13

#define LCD_DC                 8
#define LCD_CS                 9
#define LCD_CLK                10
#define LCD_MOSI               11
#define LCD_MISO               12
#define LCD_RST                14

#define LCD_BL                 2

void bsp_init(void);