idf_component_register(SRCS
    "key_event.cpp"
    "keymap.cpp"
    "keyboard_host.cpp"
    "keyboard_host_control.cpp"
    "keybinding.cpp"
    "space_hold_detector.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        usb
        usb_host_hid
        esp_timer
        esp_system
)
