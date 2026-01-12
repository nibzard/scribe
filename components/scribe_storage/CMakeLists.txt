idf_component_register(SRCS
    "storage_manager.cpp"
    "project_library.cpp"
    "autosave.cpp"
    "recovery.cpp"
    "session_state.cpp"
    "settings_store.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        fatfs
        json
        esp_system
        nvs_flash
)
