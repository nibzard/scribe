// Unit tests for Undo Stack implementation
// Tests undo/redo functionality with commands

#include <unity.h>
#include "../components/scribe_editor/undo_stack.h"
#include "../components/scribe_editor/piece_table.h"
#include <string>

// Test fixture
static PieceTable* pt = nullptr;
static UndoStack* undo = nullptr;

void setUp(void) {
    pt = new PieceTable();
    pt->init();
    undo = new UndoStack();
}

void tearDown(void) {
    delete undo;
    delete pt;
    pt = nullptr;
    undo = nullptr;
}

// ============================================================================
// Basic Undo Tests
// ============================================================================

void test_undo_single_insert(void) {
    pt->insert(0, 'a');

    undo->saveState(pt);

    pt->insert(1, 'b');

    undo->undo(pt);

    TEST_ASSERT_EQUAL_STRING("a", pt->getText().c_str());
}

void test_undo_multiple_inserts(void) {
    pt->insert(0, 'a');
    undo->saveState(pt);

    pt->insert(1, 'b');
    undo->saveState(pt);

    pt->insert(2, 'c');
    undo->saveState(pt);

    // Undo to "ab"
    undo->undo(pt);
    TEST_ASSERT_EQUAL_STRING("ab", pt->getText().c_str());

    // Undo to "a"
    undo->undo(pt);
    TEST_ASSERT_EQUAL_STRING("a", pt->getText().c_str());
}

void test_undo_delete(void) {
    pt->insert(0, 'a');
    pt->insert(1, 'b');
    pt->insert(2, 'c');

    undo->saveState(pt);

    pt->erase(1, 1);  // Delete 'b'

    undo->undo(pt);

    TEST_ASSERT_EQUAL_STRING("abc", pt->getText().c_str());
}

// ============================================================================
// Redo Tests
// ============================================================================

void test_redo_after_undo(void) {
    pt->insert(0, 'a');
    undo->saveState(pt);

    pt->insert(1, 'b');
    undo->saveState(pt);

    // Undo
    undo->undo(pt);
    TEST_ASSERT_EQUAL_STRING("a", pt->getText().c_str());

    // Redo
    undo->redo(pt);
    TEST_ASSERT_EQUAL_STRING("ab", pt->getText().c_str());
}

void test_redo_multiple(void) {
    pt->insert(0, 'a');
    undo->saveState(pt);

    pt->insert(1, 'b');
    undo->saveState(pt);

    pt->insert(2, 'c');
    undo->saveState(pt);

    // Undo twice
    undo->undo(pt);
    undo->undo(pt);

    TEST_ASSERT_EQUAL_STRING("a", pt->getText().c_str());

    // Redo twice
    undo->redo(pt);
    undo->redo(pt);

    TEST_ASSERT_EQUAL_STRING("abc", pt->getText().c_str());
}

// ============================================================================
// Undo/Redo Edge Cases
// ============================================================================

void test_undo_empty_stack(void) {
    pt->insert(0, 'a');

    // Try to undo with empty stack
    undo->undo(pt);

    // Should still be "a"
    TEST_ASSERT_EQUAL_STRING("a", pt->getText().c_str());
}

void test_redo_empty_stack(void) {
    pt->insert(0, 'a');

    // Try to redo with empty redo stack
    undo->redo(pt);

    // Should still be "a"
    TEST_ASSERT_EQUAL_STRING("a", pt->getText().c_str());
}

void test_new_action_clears_redo(void) {
    pt->insert(0, 'a');
    undo->saveState(pt);

    pt->insert(1, 'b');
    undo->saveState(pt);

    // Undo
    undo->undo(pt);
    TEST_ASSERT_EQUAL_STRING("a", pt->getText().c_str());

    // New action
    pt->insert(1, 'x');
    undo->saveState(pt);

    // Redo should be cleared
    undo->redo(pt);

    TEST_ASSERT_EQUAL_STRING("ax", pt->getText().c_str());
}

// ============================================================================
// State Tests
// ============================================================================

void test_can_undo(void) {
    TEST_ASSERT_FALSE(undo->canUndo());

    pt->insert(0, 'a');
    undo->saveState(pt);

    TEST_ASSERT_TRUE(undo->canUndo());
}

void test_can_redo(void) {
    TEST_ASSERT_FALSE(undo->canRedo());

    pt->insert(0, 'a');
    undo->saveState(pt);

    pt->insert(1, 'b');
    undo->saveState(pt);

    undo->undo(pt);

    TEST_ASSERT_TRUE(undo->canRedo());
}

void test_clear(void) {
    pt->insert(0, 'a');
    undo->saveState(pt);

    pt->insert(1, 'b');
    undo->saveState(pt);

    undo->undo(pt);

    TEST_ASSERT_TRUE(undo->canRedo());

    undo->clear();

    TEST_ASSERT_FALSE(undo->canUndo());
    TEST_ASSERT_FALSE(undo->canRedo());
}

// ============================================================================
// Large Text Tests
// ============================================================================

void test_undo_large_text(void) {
    // Insert 100 chars
    for (int i = 0; i < 100; i++) {
        pt->insert(i, 'a');
    }
    undo->saveState(pt);

    // Insert 100 more
    for (int i = 100; i < 200; i++) {
        pt->insert(i, 'b');
    }

    TEST_ASSERT_EQUAL(200, pt->length());

    undo->undo(pt);

    TEST_ASSERT_EQUAL(100, pt->length());
}

// ============================================================================
// Complex Edit Sequence Tests
// ============================================================================

void test_complex_sequence(void) {
    // Initial: "abc"
    pt->insert(0, 'a');
    pt->insert(1, 'b');
    pt->insert(2, 'c');
    undo->saveState(pt);

    // Insert: "abcd"
    pt->insert(3, 'd');
    undo->saveState(pt);

    // Delete: "abd"
    pt->erase(2, 1);
    undo->saveState(pt);

    // Insert: "abXe"
    pt->insert(2, 'X');
    pt->insert(3, 'e');
    undo->saveState(pt);

    TEST_ASSERT_EQUAL_STRING("abXe", pt->getText().c_str());

    // Undo all
    undo->undo(pt);  // -> "abd"
    undo->undo(pt);  // -> "abcd"
    undo->undo(pt);  // -> "abc"

    TEST_ASSERT_EQUAL_STRING("abc", pt->getText().c_str());

    // Redo all
    undo->redo(pt);  // -> "abcd"
    undo->redo(pt);  // -> "abd"
    undo->redo(pt);  // -> "abXe"

    TEST_ASSERT_EQUAL_STRING("abXe", pt->getText().c_str());
}

// Main test runner
int app_main(void) {
    UNITY_BEGIN();

    // Basic undo tests
    RUN_TEST(test_undo_single_insert);
    RUN_TEST(test_undo_multiple_inserts);
    RUN_TEST(test_undo_delete);

    // Redo tests
    RUN_TEST(test_redo_after_undo);
    RUN_TEST(test_redo_multiple);

    // Edge cases
    RUN_TEST(test_undo_empty_stack);
    RUN_TEST(test_redo_empty_stack);
    RUN_TEST(test_new_action_clears_redo);

    // State tests
    RUN_TEST(test_can_undo);
    RUN_TEST(test_can_redo);
    RUN_TEST(test_clear);

    // Large text tests
    RUN_TEST(test_undo_large_text);

    // Complex sequences
    RUN_TEST(test_complex_sequence);

    return UNITY_END();
}
