cmake_minimum_required(VERSION 3.16)

if (COMMAND idf_component_test)
    # ESP-IDF component tests
    idf_component_test(
        SRCS "test_piece_table.cpp"
             "test_undo_stack.cpp"
             "test_editor_core.cpp"
        INCLUDE_DIRS "."
        REQUIRES
            scribe_editor
            unity
    )
else()
    # Host-based tests
    project(scribe_host_tests LANGUAGES C CXX)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    if (NOT UNITY_SRC)
        if (DEFINED ENV{IDF_PATH})
            set(UNITY_SRC "$ENV{IDF_PATH}/components/unity/unity/src/unity.c")
        endif()
    endif()

    if (NOT UNITY_SRC OR NOT EXISTS "${UNITY_SRC}")
        message(FATAL_ERROR "UNITY_SRC not set or not found. Set UNITY_SRC or IDF_PATH to an ESP-IDF checkout.")
    endif()

    get_filename_component(UNITY_DIR "${UNITY_SRC}" DIRECTORY)
    set(SCRIBE_EDITOR_DIR "${CMAKE_CURRENT_LIST_DIR}/../components/scribe_editor")

    set(EDITOR_SRCS
        "${SCRIBE_EDITOR_DIR}/piece_table.cpp"
        "${SCRIBE_EDITOR_DIR}/undo_stack.cpp"
    )
    set(EDITOR_CORE_SRCS
        ${EDITOR_SRCS}
        "${SCRIBE_EDITOR_DIR}/editor_core.cpp"
        "${SCRIBE_EDITOR_DIR}/selection.cpp"
    )

    enable_testing()

    function(add_scribe_test target source)
        add_executable(${target} "${source}" ${UNITY_SRC} ${ARGN})
        target_include_directories(${target} PRIVATE "${SCRIBE_EDITOR_DIR}" "${UNITY_DIR}")
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    add_scribe_test(test_piece_table "${CMAKE_CURRENT_LIST_DIR}/test_piece_table.cpp" ${EDITOR_SRCS})
    add_scribe_test(test_undo_stack "${CMAKE_CURRENT_LIST_DIR}/test_undo_stack.cpp" ${EDITOR_SRCS})
    add_scribe_test(test_editor_core "${CMAKE_CURRENT_LIST_DIR}/test_editor_core.cpp" ${EDITOR_CORE_SRCS})
endif()
