# Unit tests for Scribe components
# Run with: idf.py build && idf.py --test-component all

idf_component_test(
    SRCS "test_piece_table.cpp"
           "test_undo_stack.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        scribe_editor
        unity
)