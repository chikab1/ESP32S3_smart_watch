idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES bsp lvgl_port lvgl wifi_manager battery app services)