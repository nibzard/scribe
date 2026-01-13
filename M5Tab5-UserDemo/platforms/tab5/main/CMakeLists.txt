file(GLOB_RECURSE APP_LAYER_SRCS
    ../../../app/*.c
    ../../../app/*.cc
    ../../../app/*.cpp
)

set(APP_LAYER_INCS
    ../../../app
)

file(GLOB_RECURSE MY_HAL_SRCS
    ./hal/*.c
    ./hal/*.cc
    ./hal/*.cpp
)

idf_component_register(SRCS "app_main.cpp" ${APP_LAYER_SRCS} ${MY_HAL_SRCS}
                    INCLUDE_DIRS "." ${APP_LAYER_INCS}
                    EMBED_TXTFILES "../audio/canon_in_d.mp3" "../audio/startup_sfx.mp3" "../audio/shutdown_sfx.mp3")
