idf_component_register(SRCS
    "export_sd.cpp"
    "send_to_computer.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        usb
        fatfs
)
