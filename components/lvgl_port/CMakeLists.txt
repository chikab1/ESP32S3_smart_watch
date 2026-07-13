idf_component_register(
    SRCS
        "src/lvgl_port.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        driver
        bsp
        lcd
        cst816s
        lvgl
        ui
        esp_timer
)