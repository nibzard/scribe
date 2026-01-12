#pragma once

#include <vector>
#include <functional>
#include <cstdint>

// Command types for undo/redo
enum class CommandType : uint8_t {
    INSERT,
    DELETE,
    COMPOUND_BEGIN,  // Start compound operation
    COMPOUND_END     // End compound operation
};

// Edit command for undo/redo
struct Command {
    CommandType type;
    size_t position;           // Where edit happened
    std::string text;          // For INSERT: inserted text, for DELETE: deleted text
    size_t length;             // Length of edit

    // For compound operations
    bool is_compound = false;
};

// Undo/redo stack using command pattern
class UndoStack {
public:
    UndoStack() : undo_index_(0) {}

    // Record a command for undo
    void push(const Command& cmd);

    // Perform undo
    Command undo();

    // Perform redo
    Command redo();

    // Check if undo/redo available
    bool canUndo() const { return undo_index_ > 0; }
    bool canRedo() const { return undo_index_ < stack_.size(); }

    // Clear stack (e.g., on document load)
    void clear() {
        stack_.clear();
        undo_index_ = 0;
    }

    // Start compound operation (group multiple edits)
    void beginCompound();
    void endCompound();

private:
    std::vector<Command> stack_;
    size_t undo_index_;
    bool in_compound_ = false;
};
