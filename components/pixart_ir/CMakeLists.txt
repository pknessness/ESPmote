idf_component_register(
    SRCS "pixart_ir.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_driver_i2c
)
