#pragma once

#include "piece_table.h"
#include "undo_stack.h"
#include <string>
#include <functional>

// Cursor position
struct Cursor {
    size_t pos = 0;
    size_t line = 0;
    size_t col = 0;
};

// Selection range
struct Selection {
    size_t start = 0;
    size_t end = 0;

    bool isActive() const { return start != end; }
    size_t length() const { return (end > start) ? (end - start) : (start - end); }
    size_t min() const { return (start < end) ? start : end; }
    size_t max() const { return (start > end) ? start : end; }
};

// Snapshot for background saves
struct EditorSnapshot {
    std::string project_id;
    std::string content;
    size_t word_count;
    size_t cursor_pos;
};

// Editor mode
enum class EditorMode {
    DRAFT,      // Basic editing
    REVISE      // With replace, go to heading, etc.
};

// Main editor class
class EditorCore {
public:
    EditorCore();

    // Document operations
    void load(const std::string& content);
    std::string getText() const;
    EditorSnapshot createSnapshot(const std::string& project_id) const;

    // Cursor operations
    void moveCursor(size_t pos);
    void moveCursorRelative(int delta);
    void moveCursorLine(int delta);
    void moveCursorRelativeSelect(int delta);
    void moveCursorLineSelect(int delta);
    void moveCursorWord(int direction);  // -1 left, 1 right
    void moveCursorLineEnd();
    void moveCursorLineStart();
    void moveCursorDocumentStart();
    void moveCursorDocumentEnd();

    // Selection operations
    void selectRange(size_t start, size_t end);
    void selectWord();
    void selectAll();
    void clearSelection();
    bool hasSelection() const { return selection_.isActive(); }

    // Edit operations
    void insert(const std::string& text);
    void deleteSelection();
    void deleteChar(int direction);  // -1 backspace, 1 delete
    void deleteWord(int direction);  // -1 ctrl+backspace, 1 ctrl+delete

    // Undo/redo
    void undo();
    void redo();
    bool canUndo() const { return undo_stack_.canUndo(); }
    bool canRedo() const { return undo_stack_.canRedo(); }

    // Find
    struct Match {
        size_t start;
        size_t end;
    };
    std::vector<Match> find(const std::string& query) const;
    size_t findNext(const std::string& query, size_t from) const;
    size_t findPrev(const std::string& query, size_t from) const;

    // State accessors
    const Cursor& getCursor() const { return cursor_; }
    const Selection& getSelection() const { return selection_; }
    size_t getLength() const { return piece_table_.length(); }
    size_t getLineCount() const { return line_count_; }
    size_t getWordCount() const { return word_count_; }
    EditorMode getMode() const { return mode_; }
    void setMode(EditorMode mode) { mode_ = mode; }

    // Get text at cursor (for AI, etc.)
    std::string getTextAroundCursor(size_t chars_before = 500, size_t chars_after = 500) const;

private:
    PieceTable piece_table_;
    UndoStack undo_stack_;
    Cursor cursor_;
    Selection selection_;
    EditorMode mode_;

    size_t line_count_ = 0;
    size_t word_count_ = 0;

    // Update cursor line/col from position
    void updateCursorLineCol();
    size_t calculateLineMoveTarget(int delta) const;

    // Update word count
    void updateWordCount();
};
