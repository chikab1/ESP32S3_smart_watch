idf_component_register(
    SRCS
        "src/imu_task.c"
        "src/touch_task.c"
        "src/ui_task.c"
        "src/app_task.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        driver
        bsp
        qmi8658
        cst816s
)