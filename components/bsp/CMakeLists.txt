idf_component_register(
    SRCS
        "src/bsp_board.c"
        "src/bsp_i2c.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        driver
)