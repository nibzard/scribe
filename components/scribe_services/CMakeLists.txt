idf_component_register(SRCS
    "battery.cpp"
    "power_manager.cpp"
    "rtc_time.cpp"
    "wifi_manager.cpp"
    "ai_assist.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        esp_adc
        esp_common
        esp_wifi
        esp_event
        nvs_flash
        esp_http_client
        json
)
