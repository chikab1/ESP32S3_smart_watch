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

    INCLUDE_DIRS
        "include"

    REQUIRES
        lvgl
)