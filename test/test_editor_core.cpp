// Unit tests for EditorCore implementation
// Tests cursor movement, selection, editing, undo/redo, and search helpers

#include <unity.h>
#include "editor_core.h"
#include <string>

// Unity test fixtures
void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Load and State Tests
// ============================================================================

void test_load_sets_counts_and_cursor(void) {
    EditorCore editor;
    editor.load("one two\nthree");

    TEST_ASSERT_EQUAL_STRING("one two\nthree", editor.getText().c_str());

    const Cursor& cursor = editor.getCursor();
    TEST_ASSERT_EQUAL(0, cursor.pos);
    TEST_ASSERT_EQUAL(0, cursor.line);
    TEST_ASSERT_EQUAL(0, cursor.col);

    TEST_ASSERT_EQUAL(2, editor.getLineCount());
    TEST_ASSERT_EQUAL(3, editor.getWordCount());
}

// ============================================================================
// Cursor Movement Tests
// ============================================================================

void test_move_cursor_relative_bounds(void) {
    EditorCore editor;
    editor.load("abc");

    editor.moveCursorRelative(-1);
    TEST_ASSERT_EQUAL(0, editor.getCursor().pos);

    editor.moveCursorRelative(5);
    TEST_ASSERT_EQUAL(3, editor.getCursor().pos);

    editor.moveCursorRelative(-2);
    TEST_ASSERT_EQUAL(1, editor.getCursor().pos);
}

void test_move_cursor_word(void) {
    EditorCore editor;
    editor.load("hello world");

    editor.moveCursor(0);
    editor.moveCursorWord(1);
    TEST_ASSERT_EQUAL(6, editor.getCursor().pos);

    editor.moveCursorWord(-1);
    TEST_ASSERT_EQUAL(0, editor.getCursor().pos);
}

void test_move_cursor_line_clamps_column(void) {
    EditorCore editor;
    editor.load("abcd\nxy\n12345");

    editor.moveCursor(12);  // line 2, col 4
    const Cursor& start = editor.getCursor();
    TEST_ASSERT_EQUAL(2, start.line);
    TEST_ASSERT_EQUAL(4, start.col);

    editor.moveCursorLine(-1);
    const Cursor& after = editor.getCursor();
    TEST_ASSERT_EQUAL(1, after.line);
    TEST_ASSERT_EQUAL(2, after.col);
    TEST_ASSERT_EQUAL(7, after.pos);  // end of "xy"
}

void test_move_cursor_line_start_end(void) {
    EditorCore editor;
    editor.load("abc\ndef");

    editor.moveCursor(2);
    editor.moveCursorLineStart();
    TEST_ASSERT_EQUAL(0, editor.getCursor().pos);

    editor.moveCursor(1);
    editor.moveCursorLineEnd();
    TEST_ASSERT_EQUAL(3, editor.getCursor().pos);

    editor.moveCursor(5);
    editor.moveCursorLineStart();
    TEST_ASSERT_EQUAL(4, editor.getCursor().pos);
    editor.moveCursorLineEnd();
    TEST_ASSERT_EQUAL(7, editor.getCursor().pos);
}

// ============================================================================
// Selection Tests
// ============================================================================

void test_select_range_and_text(void) {
    EditorCore editor;
    editor.load("hello world");

    editor.selectRange(6, 11);
    TEST_ASSERT_TRUE(editor.hasSelection());
    TEST_ASSERT_EQUAL_STRING("world", editor.getSelectedText().c_str());

    editor.selectRange(11, 6);
    TEST_ASSERT_EQUAL_STRING("world", editor.getSelectedText().c_str());
}

void test_select_word_and_all(void) {
    EditorCore editor;
    editor.load("hello, world");

    editor.moveCursor(1);
    editor.selectWord();
    TEST_ASSERT_EQUAL_STRING("hello", editor.getSelectedText().c_str());

    editor.selectAll();
    TEST_ASSERT_EQUAL_STRING("hello, world", editor.getSelectedText().c_str());
}

void test_move_cursor_relative_select(void) {
    EditorCore editor;
    editor.load("abcd");

    editor.moveCursor(1);
    editor.moveCursorRelativeSelect(2);
    TEST_ASSERT_TRUE(editor.hasSelection());
    TEST_ASSERT_EQUAL_STRING("bc", editor.getSelectedText().c_str());
    TEST_ASSERT_EQUAL(3, editor.getCursor().pos);

    editor.moveCursorRelativeSelect(-2);
    TEST_ASSERT_FALSE(editor.hasSelection());
    TEST_ASSERT_EQUAL(1, editor.getCursor().pos);
}

void test_move_cursor_line_select(void) {
    EditorCore editor;
    editor.load("abcd\nxy\n12345");

    editor.moveCursor(2);
    editor.moveCursorLineSelect(1);
    TEST_ASSERT_TRUE(editor.hasSelection());
    TEST_ASSERT_EQUAL_STRING("cd\nxy", editor.getSelectedText().c_str());
    TEST_ASSERT_EQUAL(7, editor.getCursor().pos);
}

// ============================================================================
// Edit Tests
// ============================================================================

void test_insert_replaces_selection(void) {
    EditorCore editor;
    editor.load("hello world");

    editor.selectRange(0, 5);
    editor.insert("hi");

    TEST_ASSERT_EQUAL_STRING("hi world", editor.getText().c_str());
    TEST_ASSERT_FALSE(editor.hasSelection());
    TEST_ASSERT_EQUAL(2, editor.getCursor().pos);
}

void test_delete_selection(void) {
    EditorCore editor;
    editor.load("hello world");

    editor.selectRange(5, 11);
    editor.deleteSelection();

    TEST_ASSERT_EQUAL_STRING("hello", editor.getText().c_str());
    TEST_ASSERT_FALSE(editor.hasSelection());
    TEST_ASSERT_EQUAL(5, editor.getCursor().pos);
}

void test_delete_char_backspace(void) {
    EditorCore editor;
    editor.load("abc");

    editor.moveCursor(2);
    editor.deleteChar(-1);

    TEST_ASSERT_EQUAL_STRING("ac", editor.getText().c_str());
    TEST_ASSERT_EQUAL(1, editor.getCursor().pos);
}

void test_delete_char_forward(void) {
    EditorCore editor;
    editor.load("abc");

    editor.moveCursor(1);
    editor.deleteChar(1);

    TEST_ASSERT_EQUAL_STRING("ac", editor.getText().c_str());
    TEST_ASSERT_EQUAL(1, editor.getCursor().pos);
}

void test_delete_word_forward(void) {
    EditorCore editor;
    editor.load("hello, world");

    editor.moveCursor(0);
    editor.deleteWord(1);

    TEST_ASSERT_EQUAL_STRING("world", editor.getText().c_str());
    TEST_ASSERT_EQUAL(0, editor.getCursor().pos);
}

void test_delete_word_backward(void) {
    EditorCore editor;
    editor.load("hello world");

    editor.moveCursor(editor.getLength());
    editor.deleteWord(-1);

    TEST_ASSERT_EQUAL_STRING("hello ", editor.getText().c_str());
    TEST_ASSERT_EQUAL(6, editor.getCursor().pos);
}

// ============================================================================
// Undo/Redo Tests
// ============================================================================

void test_undo_redo_insert_sequence(void) {
    EditorCore editor;
    editor.load("");

    editor.insert("abc");
    editor.insert("def");
    TEST_ASSERT_EQUAL_STRING("abcdef", editor.getText().c_str());

    editor.undo();
    TEST_ASSERT_EQUAL_STRING("abc", editor.getText().c_str());

    editor.undo();
    TEST_ASSERT_EQUAL_STRING("", editor.getText().c_str());

    editor.redo();
    TEST_ASSERT_EQUAL_STRING("abc", editor.getText().c_str());

    editor.redo();
    TEST_ASSERT_EQUAL_STRING("abcdef", editor.getText().c_str());
}

void test_undo_redo_delete_char(void) {
    EditorCore editor;
    editor.load("abc");

    editor.moveCursor(3);
    editor.deleteChar(-1);
    TEST_ASSERT_EQUAL_STRING("ab", editor.getText().c_str());

    editor.undo();
    TEST_ASSERT_EQUAL_STRING("abc", editor.getText().c_str());

    editor.redo();
    TEST_ASSERT_EQUAL_STRING("ab", editor.getText().c_str());
}

// ============================================================================
// Find and Utility Tests
// ============================================================================

void test_find_helpers(void) {
    EditorCore editor;
    editor.load("aba aba");

    auto matches = editor.find("aba");
    TEST_ASSERT_EQUAL(2, matches.size());
    TEST_ASSERT_EQUAL(0, matches[0].start);
    TEST_ASSERT_EQUAL(3, matches[0].end);
    TEST_ASSERT_EQUAL(4, matches[1].start);
    TEST_ASSERT_EQUAL(7, matches[1].end);

    TEST_ASSERT_EQUAL(4, editor.findNext("aba", 1));
    TEST_ASSERT_EQUAL(0, editor.findPrev("aba", 3));
    TEST_ASSERT_EQUAL(4, editor.findPrev("aba", 6));
}

void test_text_around_cursor(void) {
    EditorCore editor;
    editor.load("hello world");

    editor.moveCursor(5);
    std::string around = editor.getTextAroundCursor(2, 3);
    TEST_ASSERT_EQUAL_STRING("lo wo", around.c_str());
}

// Main test runner
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_load_sets_counts_and_cursor);
    RUN_TEST(test_move_cursor_relative_bounds);
    RUN_TEST(test_move_cursor_word);
    RUN_TEST(test_move_cursor_line_clamps_column);
    RUN_TEST(test_move_cursor_line_start_end);
    RUN_TEST(test_select_range_and_text);
    RUN_TEST(test_select_word_and_all);
    RUN_TEST(test_move_cursor_relative_select);
    RUN_TEST(test_move_cursor_line_select);
    RUN_TEST(test_insert_replaces_selection);
    RUN_TEST(test_delete_selection);
    RUN_TEST(test_delete_char_backspace);
    RUN_TEST(test_delete_char_forward);
    RUN_TEST(test_delete_word_forward);
    RUN_TEST(test_delete_word_backward);
    RUN_TEST(test_undo_redo_insert_sequence);
    RUN_TEST(test_undo_redo_delete_char);
    RUN_TEST(test_find_helpers);
    RUN_TEST(test_text_around_cursor);

    return UNITY_END();
}
