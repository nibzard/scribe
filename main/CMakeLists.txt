idf_component_register(SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        scribe_ui
        scribe_input
        scribe_editor
        scribe_storage
        scribe_export
        scribe_services
        scribe_secrets
)
