idf_component_register(SRCS
    "key_event.cpp"
    "keymap.cpp"
    "keyboard_host.cpp"
    "keybinding.cpp"
    "space_hold_detector.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        usb
        esp_system
)
