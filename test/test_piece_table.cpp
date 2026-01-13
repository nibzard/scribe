// Unit tests for Piece Table implementation
// Tests core operations: insert, delete, snapshot

#include <unity.h>
#include "piece_table.h"
#include <string>

// Unity test fixtures
void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Basic Insert Tests
// ============================================================================

void test_insert_single_char(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "a");
    TEST_ASSERT_EQUAL_STRING("a", pt.getText().c_str());
}

void test_insert_multiple_chars(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "H");
    pt.insert(1, "e");
    pt.insert(2, "l");
    pt.insert(3, "l");
    pt.insert(4, "o");
    TEST_ASSERT_EQUAL_STRING("Hello", pt.getText().c_str());
}

void test_insert_string(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "World");
    TEST_ASSERT_EQUAL_STRING("World", pt.getText().c_str());
}

void test_insert_middle(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "H");
    pt.insert(1, "l");
    pt.insert(2, "o");
    pt.insert(1, "e");
    TEST_ASSERT_EQUAL_STRING("Helo", pt.getText().c_str());
    pt.insert(2, "l");
    TEST_ASSERT_EQUAL_STRING("Hello", pt.getText().c_str());
}

// ============================================================================
// Delete Tests
// ============================================================================

void test_delete_single_char(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "a");
    pt.insert(1, "b");
    pt.insert(2, "c");
    pt.remove(1, 2);
    TEST_ASSERT_EQUAL_STRING("ac", pt.getText().c_str());
}

void test_delete_multiple_chars(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "Hello World");
    pt.remove(5, 11);
    TEST_ASSERT_EQUAL_STRING("Hello", pt.getText().c_str());
}

void test_delete_from_start(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "abcdef");
    pt.remove(0, 3);
    TEST_ASSERT_EQUAL_STRING("def", pt.getText().c_str());
}

void test_delete_from_end(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "abcdef");
    pt.remove(3, 6);
    TEST_ASSERT_EQUAL_STRING("abc", pt.getText().c_str());
}

// ============================================================================
// Length Tests
// ============================================================================

void test_empty_length(void) {
    PieceTable pt;
    pt.load("");
    TEST_ASSERT_EQUAL(0, pt.length());
}

void test_length_after_insert(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "a");
    pt.insert(1, "b");
    pt.insert(2, "c");
    TEST_ASSERT_EQUAL(3, pt.length());
}

void test_length_after_delete(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "abcde");
    pt.remove(1, 3);
    TEST_ASSERT_EQUAL(3, pt.length());
}

// ============================================================================
// Large Text Tests
// ============================================================================

void test_large_insert(void) {
    PieceTable pt;
    pt.load("");
    std::string text;
    for (int i = 0; i < 1000; i++) {
        text += 'a' + (i % 26);
    }
    pt.insert(0, text);
    TEST_ASSERT_EQUAL(1000, pt.length());
}

void test_large_delete(void) {
    PieceTable pt;
    pt.load("");
    std::string text(1000, 'a');
    pt.insert(0, text);
    pt.remove(0, 500);
    TEST_ASSERT_EQUAL(500, pt.length());
}

// ============================================================================
// Range Tests
// ============================================================================

void test_get_range(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "Hello World");
    std::string range = pt.getTextRange(0, 5);
    TEST_ASSERT_EQUAL_STRING("Hello", range.c_str());
    range = pt.getTextRange(6, 11);
    TEST_ASSERT_EQUAL_STRING("World", range.c_str());
}

void test_get_range_edges(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "abc");
    std::string range = pt.getTextRange(0, 3);
    TEST_ASSERT_EQUAL_STRING("abc", range.c_str());
    range = pt.getTextRange(1, 3);
    TEST_ASSERT_EQUAL_STRING("bc", range.c_str());
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void test_insert_clamps_to_end(void) {
    PieceTable pt;
    pt.load("abc");
    pt.insert(42, "X");
    TEST_ASSERT_EQUAL_STRING("abcX", pt.getText().c_str());
    TEST_ASSERT_EQUAL(4, pt.length());
}

void test_insert_empty_noop(void) {
    PieceTable pt;
    pt.load("abc");
    pt.insert(1, "");
    TEST_ASSERT_EQUAL_STRING("abc", pt.getText().c_str());
    TEST_ASSERT_EQUAL(3, pt.length());
}

void test_insert_at_start_existing_content(void) {
    PieceTable pt;
    pt.load("world");
    pt.insert(0, "hello ");
    TEST_ASSERT_EQUAL_STRING("hello world", pt.getText().c_str());
    TEST_ASSERT_EQUAL(11, pt.length());
}

void test_remove_out_of_range_noop(void) {
    PieceTable pt;
    pt.load("abc");
    pt.remove(2, 2);
    pt.remove(5, 10);
    TEST_ASSERT_EQUAL_STRING("abc", pt.getText().c_str());
    TEST_ASSERT_EQUAL(3, pt.length());
}

void test_remove_clamps_end(void) {
    PieceTable pt;
    pt.load("abcdef");
    pt.remove(3, 20);
    TEST_ASSERT_EQUAL_STRING("abc", pt.getText().c_str());
    TEST_ASSERT_EQUAL(3, pt.length());
}

void test_get_range_out_of_bounds(void) {
    PieceTable pt;
    pt.load("abc");
    std::string range = pt.getTextRange(5, 10);
    TEST_ASSERT_EQUAL_STRING("", range.c_str());
    range = pt.getTextRange(3, 3);
    TEST_ASSERT_EQUAL_STRING("", range.c_str());
}

// ============================================================================
// Snapshot Tests
// ============================================================================

void test_snapshot_preserves_content(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "Hello World");
    PieceTableSnapshot snapshot = pt.createSnapshot();
    pt.remove(5, 11);
    TEST_ASSERT_EQUAL(11, snapshot.total_length);
}

void test_snapshot_after_edits(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "abc");
    PieceTableSnapshot snap1 = pt.createSnapshot();
    pt.insert(3, "de");
    PieceTableSnapshot snap2 = pt.createSnapshot();
    TEST_ASSERT_EQUAL(3, snap1.total_length);
    TEST_ASSERT_EQUAL(5, snap2.total_length);
}

void test_snapshot_add_buffer_copy_on_write(void) {
    PieceTable pt;
    pt.load("");
    pt.insert(0, "hello");
    PieceTableSnapshot snap = pt.createSnapshot();

    pt.insert(5, " world");
    TEST_ASSERT_EQUAL_STRING("hello", snap.add_buffer->c_str());
    TEST_ASSERT_EQUAL_STRING("hello world", pt.getText().c_str());
}

// ============================================================================
// Compact Tests
// ============================================================================

void test_compact_resets_pieces(void) {
    PieceTable pt;
    pt.load("abc");
    pt.insert(1, "X");
    pt.remove(2, 3);

    std::string before = pt.getText();
    pt.compact();

    TEST_ASSERT_EQUAL_STRING(before.c_str(), pt.getText().c_str());
    const auto& pieces = pt.getPieces();
    TEST_ASSERT_EQUAL(1, pieces.size());
    if (!pieces.empty()) {
        TEST_ASSERT_TRUE(pieces[0].type == Piece::Type::ORIGINAL);
        TEST_ASSERT_EQUAL(before.length(), pieces[0].length);
    }
}

// ============================================================================
// Load Tests
// ============================================================================

void test_load_initial_content(void) {
    PieceTable pt;
    pt.load("Initial content");
    TEST_ASSERT_EQUAL_STRING("Initial content", pt.getText().c_str());
    TEST_ASSERT_EQUAL(15, pt.length());
}

void test_load_empty(void) {
    PieceTable pt;
    pt.load("");
    TEST_ASSERT_EQUAL_STRING("", pt.getText().c_str());
    TEST_ASSERT_EQUAL(0, pt.length());
}

// Main test runner
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_insert_single_char);
    RUN_TEST(test_insert_multiple_chars);
    RUN_TEST(test_insert_string);
    RUN_TEST(test_insert_middle);
    RUN_TEST(test_delete_single_char);
    RUN_TEST(test_delete_multiple_chars);
    RUN_TEST(test_delete_from_start);
    RUN_TEST(test_delete_from_end);
    RUN_TEST(test_empty_length);
    RUN_TEST(test_length_after_insert);
    RUN_TEST(test_length_after_delete);
    RUN_TEST(test_large_insert);
    RUN_TEST(test_large_delete);
    RUN_TEST(test_get_range);
    RUN_TEST(test_get_range_edges);
    RUN_TEST(test_insert_clamps_to_end);
    RUN_TEST(test_insert_empty_noop);
    RUN_TEST(test_insert_at_start_existing_content);
    RUN_TEST(test_remove_out_of_range_noop);
    RUN_TEST(test_remove_clamps_end);
    RUN_TEST(test_get_range_out_of_bounds);
    RUN_TEST(test_snapshot_preserves_content);
    RUN_TEST(test_snapshot_after_edits);
    RUN_TEST(test_snapshot_add_buffer_copy_on_write);
    RUN_TEST(test_compact_resets_pieces);
    RUN_TEST(test_load_initial_content);
    RUN_TEST(test_load_empty);

    return UNITY_END();
}
