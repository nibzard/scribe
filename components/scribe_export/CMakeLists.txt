idf_component_register(SRCS
    "export_sd.cpp"
    "send_to_computer.cpp"
    "usb_hid_device.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        usb
        esp_tinyusb
        fatfs
        scribe_input
)
