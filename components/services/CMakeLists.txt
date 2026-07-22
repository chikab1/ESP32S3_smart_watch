idf_component_register(
    SRCS
        "src/watch_event.c"
        "src/power_service.c"
        "src/settings_service.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        driver
        esp_event
        esp_timer
        nvs_flash
        bsp
        battery
        wifi_manager
        cst816s
        lvgl_port
        app
)