// Unit tests for Undo Stack implementation
// Tests undo/redo functionality using Command pattern

#include <unity.h>
#include "undo_stack.h"
#include "piece_table.h"
#include <string>

// Unity test fixtures
void setUp(void) {}
void tearDown(void) {}

// Helper to apply insert command
void applyInsert(PieceTable* table, const Command& cmd) {
    table->insert(cmd.position, cmd.text);
}

// Helper to apply delete command
void applyDelete(PieceTable* table, const Command& cmd) {
    table->remove(cmd.position, cmd.position + cmd.length);
}

// Helper to undo insert
void undoInsert(PieceTable* table, const Command& cmd) {
    table->remove(cmd.position, cmd.position + cmd.length);
}

// Helper to undo delete (restore deleted text)
void undoDelete(PieceTable* table, const Command& cmd) {
    table->insert(cmd.position, cmd.text);
}

// ============================================================================
// Basic Undo Tests
// ============================================================================

void test_undo_single_insert(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    pt.insert(0, "a");
    undo.push(Command{CommandType::INSERT, 0, "a", 1});

    pt.insert(1, "b");
    undo.push(Command{CommandType::INSERT, 1, "b", 1});

    // Undo the insert of "b" at position 1
    Command undo_cmd = undo.undo();
    undoInsert(&pt, undo_cmd);
    TEST_ASSERT_EQUAL_STRING("a", pt.getText().c_str());
}

void test_undo_multiple_inserts(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    pt.insert(0, "a");
    undo.push(Command{CommandType::INSERT, 0, "a", 1});

    pt.insert(1, "b");
    undo.push(Command{CommandType::INSERT, 1, "b", 1});

    pt.insert(2, "c");
    undo.push(Command{CommandType::INSERT, 2, "c", 1});

    Command cmd = undo.undo();
    undoInsert(&pt, cmd);
    TEST_ASSERT_EQUAL_STRING("ab", pt.getText().c_str());

    cmd = undo.undo();
    undoInsert(&pt, cmd);
    TEST_ASSERT_EQUAL_STRING("a", pt.getText().c_str());
}

void test_undo_delete(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    pt.insert(0, "abc");
    undo.push(Command{CommandType::INSERT, 0, "abc", 3});

    pt.remove(1, 2);
    undo.push(Command{CommandType::DELETE, 1, "b", 1});

    Command cmd = undo.undo();
    undoDelete(&pt, cmd);
    TEST_ASSERT_EQUAL_STRING("abc", pt.getText().c_str());
}

// ============================================================================
// Redo Tests
// ============================================================================

void test_redo_after_undo(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    pt.insert(0, "a");
    undo.push(Command{CommandType::INSERT, 0, "a", 1});

    pt.insert(1, "b");
    undo.push(Command{CommandType::INSERT, 1, "b", 1});

    Command cmd = undo.undo();
    undoInsert(&pt, cmd);
    TEST_ASSERT_EQUAL_STRING("a", pt.getText().c_str());

    Command redo_cmd = undo.redo();
    applyInsert(&pt, redo_cmd);
    TEST_ASSERT_EQUAL_STRING("ab", pt.getText().c_str());
}

void test_redo_multiple(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    pt.insert(0, "a");
    undo.push(Command{CommandType::INSERT, 0, "a", 1});

    pt.insert(1, "b");
    undo.push(Command{CommandType::INSERT, 1, "b", 1});

    pt.insert(2, "c");
    undo.push(Command{CommandType::INSERT, 2, "c", 1});

    Command cmd = undo.undo();
    undoInsert(&pt, cmd);
    cmd = undo.undo();
    undoInsert(&pt, cmd);

    TEST_ASSERT_EQUAL_STRING("a", pt.getText().c_str());

    Command redo_cmd = undo.redo();
    applyInsert(&pt, redo_cmd);
    redo_cmd = undo.redo();
    applyInsert(&pt, redo_cmd);

    TEST_ASSERT_EQUAL_STRING("abc", pt.getText().c_str());
}

// ============================================================================
// Undo/Redo Edge Cases
// ============================================================================

void test_undo_empty_stack(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    pt.insert(0, "a");
    TEST_ASSERT_FALSE(undo.canUndo());
    TEST_ASSERT_EQUAL_STRING("a", pt.getText().c_str());
}

void test_redo_empty_stack(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    pt.insert(0, "a");
    TEST_ASSERT_FALSE(undo.canRedo());
    TEST_ASSERT_EQUAL_STRING("a", pt.getText().c_str());
}

void test_new_action_clears_redo(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    pt.insert(0, "a");
    undo.push(Command{CommandType::INSERT, 0, "a", 1});

    pt.insert(1, "b");
    undo.push(Command{CommandType::INSERT, 1, "b", 1});

    Command cmd = undo.undo();
    undoInsert(&pt, cmd);
    TEST_ASSERT_EQUAL_STRING("a", pt.getText().c_str());
    TEST_ASSERT_TRUE(undo.canRedo());

    pt.insert(1, "x");
    undo.push(Command{CommandType::INSERT, 1, "x", 1});

    TEST_ASSERT_EQUAL_STRING("ax", pt.getText().c_str());
    TEST_ASSERT_FALSE(undo.canRedo());
}

// ============================================================================
// State Tests
// ============================================================================

void test_can_undo(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    TEST_ASSERT_FALSE(undo.canUndo());

    pt.insert(0, "a");
    undo.push(Command{CommandType::INSERT, 0, "a", 1});

    TEST_ASSERT_TRUE(undo.canUndo());
}

void test_can_redo(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    TEST_ASSERT_FALSE(undo.canRedo());

    pt.insert(0, "a");
    undo.push(Command{CommandType::INSERT, 0, "a", 1});

    pt.insert(1, "b");
    undo.push(Command{CommandType::INSERT, 1, "b", 1});

    undo.undo();

    TEST_ASSERT_TRUE(undo.canRedo());
}

void test_clear(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    pt.insert(0, "a");
    undo.push(Command{CommandType::INSERT, 0, "a", 1});

    pt.insert(1, "b");
    undo.push(Command{CommandType::INSERT, 1, "b", 1});

    undo.undo();

    TEST_ASSERT_TRUE(undo.canRedo());

    undo.clear();

    TEST_ASSERT_FALSE(undo.canUndo());
    TEST_ASSERT_FALSE(undo.canRedo());
}

// ============================================================================
// Large Text Tests
// ============================================================================

void test_undo_large_text(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    std::string text1(100, 'a');
    pt.insert(0, text1);
    undo.push(Command{CommandType::INSERT, 0, text1, 100});

    std::string text2(100, 'b');
    pt.insert(100, text2);

    TEST_ASSERT_EQUAL(200, pt.length());

    Command cmd = undo.undo();
    undoInsert(&pt, cmd);

    TEST_ASSERT_EQUAL(100, pt.length());
}

// ============================================================================
// Complex Edit Sequence Tests
// ============================================================================

void test_complex_sequence(void) {
    PieceTable pt;
    pt.load("");
    UndoStack undo;

    pt.insert(0, "abc");
    undo.push(Command{CommandType::INSERT, 0, "abc", 3});

    pt.insert(3, "d");
    undo.push(Command{CommandType::INSERT, 3, "d", 1});

    pt.remove(2, 3);
    undo.push(Command{CommandType::DELETE, 2, "c", 1});

    pt.insert(2, "X");
    undo.push(Command{CommandType::INSERT, 2, "X", 1});

    pt.insert(4, "e");  // position 4 to put 'e' after 'd'
    undo.push(Command{CommandType::INSERT, 4, "e", 1});

    TEST_ASSERT_EQUAL_STRING("abXde", pt.getText().c_str());

    Command cmd = undo.undo();
    undoInsert(&pt, cmd);
    cmd = undo.undo();
    undoInsert(&pt, cmd);
    cmd = undo.undo();
    undoDelete(&pt, cmd);
    cmd = undo.undo();
    undoInsert(&pt, cmd);

    TEST_ASSERT_EQUAL_STRING("abc", pt.getText().c_str());

    Command redo_cmd = undo.redo();
    applyInsert(&pt, redo_cmd);
    redo_cmd = undo.redo();
    applyDelete(&pt, redo_cmd);  // redo a delete by applying it again
    redo_cmd = undo.redo();
    applyInsert(&pt, redo_cmd);
    redo_cmd = undo.redo();
    applyInsert(&pt, redo_cmd);

    TEST_ASSERT_EQUAL_STRING("abXde", pt.getText().c_str());
}

// Main test runner
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_undo_single_insert);
    RUN_TEST(test_undo_multiple_inserts);
    RUN_TEST(test_undo_delete);
    RUN_TEST(test_redo_after_undo);
    RUN_TEST(test_redo_multiple);
    RUN_TEST(test_undo_empty_stack);
    RUN_TEST(test_redo_empty_stack);
    RUN_TEST(test_new_action_clears_redo);
    RUN_TEST(test_can_undo);
    RUN_TEST(test_can_redo);
    RUN_TEST(test_clear);
    RUN_TEST(test_undo_large_text);
    RUN_TEST(test_complex_sequence);

    return UNITY_END();
}
