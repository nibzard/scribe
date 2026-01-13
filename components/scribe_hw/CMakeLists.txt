idf_component_register(SRCS
    "tab5_i2c.cpp"
    "tab5_io_expander.cpp"
    "tab5_ina226.cpp"
    "tab5_rx8130.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_i2c
        esp_driver_gpio
        esp_common
        freertos
)
