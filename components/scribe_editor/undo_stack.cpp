#include "undo_stack.h"

void UndoStack::push(const Command& cmd) {
    // Remove any redo history
    if (undo_index_ < stack_.size()) {
        stack_.resize(undo_index_);
    }
    stack_.push_back(cmd);
    undo_index_++;
}

Command UndoStack::undo() {
    if (!canUndo()) return Command{CommandType::INSERT, 0, "", 0};
    return stack_[--undo_index_];
}

Command UndoStack::redo() {
    if (!canRedo()) return Command{CommandType::INSERT, 0, "", 0};
    return stack_[undo_index_++];
}

void UndoStack::beginCompound() {
    in_compound_ = true;
}

void UndoStack::endCompound() {
    in_compound_ = false;
}
