// Unit tests for Piece Table implementation
// Tests core operations: insert, delete, snapshot, serialization

#include <unity.h>
#include "../components/scribe_editor/piece_table.h"
#include <string>

// Test fixture
static PieceTable* pt = nullptr;

void setUp(void) {
    pt = new PieceTable();
    pt->init();
}

void tearDown(void) {
    delete pt;
    pt = nullptr;
}

// ============================================================================
// Basic Insert Tests
// ============================================================================

void test_insert_single_char(void) {
    pt->insert(0, 'a');
    TEST_ASSERT_EQUAL_STRING("a", pt->getText().c_str());
}

void test_insert_multiple_chars(void) {
    pt->insert(0, 'H');
    pt->insert(1, 'e');
    pt->insert(2, 'l');
    pt->insert(3, 'l');
    pt->insert(4, 'o');

    TEST_ASSERT_EQUAL_STRING("Hello", pt->getText().c_str());
}

void test_insert_string(void) {
    std::string text = "World";
    for (size_t i = 0; i < text.length(); i++) {
        pt->insert(i, text[i]);
    }

    TEST_ASSERT_EQUAL_STRING("World", pt->getText().c_str());
}

void test_insert_middle(void) {
    // Start with "Hlo"
    pt->insert(0, 'H');
    pt->insert(1, 'l');
    pt->insert(2, 'o');

    // Insert 'e' at position 1
    pt->insert(1, 'e');

    TEST_ASSERT_EQUAL_STRING("Helo", pt->getText().c_str());

    // Insert 'l' at position 2
    pt->insert(2, 'l');

    TEST_ASSERT_EQUAL_STRING("Hello", pt->getText().c_str());
}

// ============================================================================
// Delete Tests
// ============================================================================

void test_delete_single_char(void) {
    pt->insert(0, 'a');
    pt->insert(1, 'b');
    pt->insert(2, 'c');

    pt->erase(1, 1);

    TEST_ASSERT_EQUAL_STRING("ac", pt->getText().c_str());
}

void test_delete_multiple_chars(void) {
    std::string text = "Hello World";
    for (size_t i = 0; i < text.length(); i++) {
        pt->insert(i, text[i]);
    }

    // Delete " World"
    pt->erase(5, 6);

    TEST_ASSERT_EQUAL_STRING("Hello", pt->getText().c_str());
}

void test_delete_from_start(void) {
    std::string text = "abcdef";
    for (size_t i = 0; i < text.length(); i++) {
        pt->insert(i, text[i]);
    }

    pt->erase(0, 3);

    TEST_ASSERT_EQUAL_STRING("def", pt->getText().c_str());
}

void test_delete_from_end(void) {
    std::string text = "abcdef";
    for (size_t i = 0; i < text.length(); i++) {
        pt->insert(i, text[i]);
    }

    pt->erase(3, 3);

    TEST_ASSERT_EQUAL_STRING("abc", pt->getText().c_str());
}

// ============================================================================
// Length Tests
// ============================================================================

void test_empty_length(void) {
    TEST_ASSERT_EQUAL(0, pt->length());
}

void test_length_after_insert(void) {
    pt->insert(0, 'a');
    pt->insert(1, 'b');
    pt->insert(2, 'c');

    TEST_ASSERT_EQUAL(3, pt->length());
}

void test_length_after_delete(void) {
    std::string text = "abcde";
    for (size_t i = 0; i < text.length(); i++) {
        pt->insert(i, text[i]);
    }

    pt->erase(1, 2);

    TEST_ASSERT_EQUAL(3, pt->length());
}

// ============================================================================
// Large Text Tests
// ============================================================================

void test_large_insert(void) {
    // Insert 1000 characters
    for (int i = 0; i < 1000; i++) {
        pt->insert(i, 'a' + (i % 26));
    }

    TEST_ASSERT_EQUAL(1000, pt->length());
}

void test_large_delete(void) {
    // Insert 1000 characters
    for (int i = 0; i < 1000; i++) {
        pt->insert(i, 'a');
    }

    // Delete half
    pt->erase(0, 500);

    TEST_ASSERT_EQUAL(500, pt->length());
}

// ============================================================================
// Snapshot Tests
// ============================================================================

void test_snapshot_preserves_content(void) {
    std::string text = "Hello World";
    for (size_t i = 0; i < text.length(); i++) {
        pt->insert(i, text[i]);
    }

    std::string snapshot = pt->createSnapshot();

    // Modify
    pt->erase(5, 6);

    // Restore from snapshot
    PieceTable restored;
    restored.init();
    restored.loadSnapshot(snapshot);

    TEST_ASSERT_EQUAL_STRING("Hello World", restored.getText().c_str());
}

void test_snapshot_after_edits(void) {
    pt->insert(0, 'a');
    pt->insert(1, 'b');
    pt->insert(2, 'c');

    std::string snapshot1 = pt->createSnapshot();

    pt->insert(3, 'd');
    pt->insert(4, 'e');

    std::string snapshot2 = pt->createSnapshot();

    // Restore first snapshot
    PieceTable restored;
    restored.init();
    restored.loadSnapshot(snapshot1);

    TEST_ASSERT_EQUAL_STRING("abc", restored.getText().c_str());

    // Restore second snapshot
    PieceTable restored2;
    restored2.init();
    restored2.loadSnapshot(snapshot2);

    TEST_ASSERT_EQUAL_STRING("abcde", restored2.getText().c_str());
}

// ============================================================================
// Word Count Tests
// ============================================================================

void test_word_count_simple(void) {
    pt->insert(0, 'H');
    pt->insert(1, 'e');
    pt->insert(2, 'l');
    pt->insert(3, 'l');
    pt->insert(4, 'o');
    pt->insert(5, ' ');
    pt->insert(6, 'W');
    pt->insert(7, 'o');
    pt->insert(8, 'r');
    pt->insert(9, 'l');
    pt->insert(10, 'd');

    TEST_ASSERT_EQUAL(2, pt->countWords());
}

void test_word_count_empty(void) {
    TEST_ASSERT_EQUAL(0, pt->countWords());
}

void test_word_count_spaces(void) {
    std::string text = "   ";
    for (size_t i = 0; i < text.length(); i++) {
        pt->insert(i, text[i]);
    }

    TEST_ASSERT_EQUAL(0, pt->countWords());
}

void test_word_count_newlines(void) {
    pt->insert(0, 'a');
    pt->insert(1, '\n');
    pt->insert(2, 'b');
    pt->insert(3, '\n');
    pt->insert(4, 'c');

    TEST_ASSERT_EQUAL(3, pt->countWords());
}

// ============================================================================
// Character Access Tests
// ============================================================================

void test_char_at(void) {
    std::string text = "Hello";
    for (size_t i = 0; i < text.length(); i++) {
        pt->insert(i, text[i]);
    }

    TEST_ASSERT_EQUAL('H', pt->charAt(0));
    TEST_ASSERT_EQUAL('e', pt->charAt(1));
    TEST_ASSERT_EQUAL('l', pt->charAt(2));
    TEST_ASSERT_EQUAL('l', pt->charAt(3));
    TEST_ASSERT_EQUAL('o', pt->charAt(4));
}

void test_char_at_out_of_bounds(void) {
    pt->insert(0, 'a');

    TEST_ASSERT_EQUAL('\0', pt->charAt(100));
}

// ============================================================================
// Range Tests
// ============================================================================

void test_get_range(void) {
    std::string text = "Hello World";
    for (size_t i = 0; i < text.length(); i++) {
        pt->insert(i, text[i]);
    }

    std::string range = pt->getRange(0, 5);
    TEST_ASSERT_EQUAL_STRING("Hello", range.c_str());

    range = pt->getRange(6, 11);
    TEST_ASSERT_EQUAL_STRING("World", range.c_str());
}

void test_get_range_edges(void) {
    std::string text = "abc";
    for (size_t i = 0; i < text.length(); i++) {
        pt->insert(i, text[i]);
    }

    std::string range = pt->getRange(0, 3);
    TEST_ASSERT_EQUAL_STRING("abc", range.c_str());

    range = pt->getRange(1, 3);
    TEST_ASSERT_EQUAL_STRING("bc", range.c_str());
}

// Main test runner
int app_main(void) {
    UNITY_BEGIN();

    // Basic insert tests
    RUN_TEST(test_insert_single_char);
    RUN_TEST(test_insert_multiple_chars);
    RUN_TEST(test_insert_string);
    RUN_TEST(test_insert_middle);

    // Delete tests
    RUN_TEST(test_delete_single_char);
    RUN_TEST(test_delete_multiple_chars);
    RUN_TEST(test_delete_from_start);
    RUN_TEST(test_delete_from_end);

    // Length tests
    RUN_TEST(test_empty_length);
    RUN_TEST(test_length_after_insert);
    RUN_TEST(test_length_after_delete);

    // Large text tests
    RUN_TEST(test_large_insert);
    RUN_TEST(test_large_delete);

    // Snapshot tests
    RUN_TEST(test_snapshot_preserves_content);
    RUN_TEST(test_snapshot_after_edits);

    // Word count tests
    RUN_TEST(test_word_count_simple);
    RUN_TEST(test_word_count_empty);
    RUN_TEST(test_word_count_spaces);
    RUN_TEST(test_word_count_newlines);

    // Character access tests
    RUN_TEST(test_char_at);
    RUN_TEST(test_char_at_out_of_bounds);

    // Range tests
    RUN_TEST(test_get_range);
    RUN_TEST(test_get_range_edges);

    return UNITY_END();
}
