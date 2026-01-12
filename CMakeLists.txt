cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "scribe")
set(PROJECT_VER "0.1.0")

# Target: ESP32-P4
set(IDF_TARGET esp32p4)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME} VERSION ${PROJECT_VER})
