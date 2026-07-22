idf_component_register(
    SRCS
        "src/ui_manager.c"
        "src/ui_utils.c"
        "src/ui_style.c"
        "src/ui_data.c"
        "src/page_home.c"
        "src/page_menu.c"
        "src/page_settings.c"
        "src/page_imu.c"
        "src/page_mqtt.c"
        "src/page_about.c"
        "src/page_system.c"
        "src/page_wifi.c"
        "src/page_wifi_connect.c"
        "src/status_bar.c"
        "src/quick_settings.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        lvgl
        wifi_manager
        battery
        app
        services
)