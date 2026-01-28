idf_component_register(SRCS "strings.cpp"
    INCLUDE_DIRS "."
    REQUIRES json
    EMBED_TXTFILES "${CMAKE_CURRENT_LIST_DIR}/../../assets/strings/en.json"
)
