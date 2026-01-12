#include "editor_core.h"
#include <algorithm>
#include <cctype>
#include <sstream>

EditorCore::EditorCore() : mode_(EditorMode::DRAFT) {
    cursor_ = Cursor{0, 0, 0};
}

void EditorCore::load(const std::string& content) {
    piece_table_.load(content);
    cursor_ = Cursor{0, 0, 0};
    selection_ = Selection{0, 0};
    undo_stack_.clear();
    updateCursorLineCol();
    updateWordCount();
}

std::string EditorCore::getText() const {
    return piece_table_.getText();
}

EditorSnapshot EditorCore::createSnapshot(const std::string& project_id) const {
    return EditorSnapshot{
        .project_id = project_id,
        .content = getText(),
        .word_count = word_count_,
        .cursor_pos = cursor_.pos
    };
}

void EditorCore::moveCursor(size_t pos) {
    if (pos > piece_table_.length()) pos = piece_table_.length();
    cursor_.pos = pos;
    clearSelection();
    updateCursorLineCol();
}

void EditorCore::moveCursorRelative(int delta) {
    size_t new_pos = cursor_.pos + delta;
    if (delta < 0 && new_pos > cursor_.pos) new_pos = 0;  // Underflow check
    moveCursor(new_pos);
}

void EditorCore::moveCursorLine(int delta) {
    moveCursor(calculateLineMoveTarget(delta));
}

void EditorCore::moveCursorRelativeSelect(int delta) {
    size_t new_pos = cursor_.pos + delta;
    if (delta < 0 && new_pos > cursor_.pos) new_pos = 0;  // Underflow check
    if (new_pos > piece_table_.length()) new_pos = piece_table_.length();

    if (!hasSelection()) {
        selection_.start = cursor_.pos;
    }
    selection_.end = new_pos;
    cursor_.pos = new_pos;
    updateCursorLineCol();
}

void EditorCore::moveCursorLineSelect(int delta) {
    size_t target_pos = calculateLineMoveTarget(delta);
    if (!hasSelection()) {
        selection_.start = cursor_.pos;
    }
    selection_.end = target_pos;
    cursor_.pos = target_pos;
    updateCursorLineCol();
}

void EditorCore::moveCursorWord(int direction) {
    std::string text = getText();
    size_t pos = cursor_.pos;
    auto is_word_char = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    };

    if (direction > 0) {  // Forward
        while (pos < text.length() && is_word_char(text[pos])) pos++;
        while (pos < text.length() && !is_word_char(text[pos]) && !std::isspace(static_cast<unsigned char>(text[pos]))) pos++;
        while (pos < text.length() && std::isspace(static_cast<unsigned char>(text[pos]))) pos++;
    } else {  // Backward
        if (pos > 0) pos--;
        while (pos > 0 && std::isspace(static_cast<unsigned char>(text[pos]))) pos--;
        while (pos > 0 && !is_word_char(text[pos]) && !std::isspace(static_cast<unsigned char>(text[pos]))) pos--;
        while (pos > 0 && is_word_char(text[pos - 1])) pos--;
    }

    moveCursor(pos);
}

void EditorCore::moveCursorLineEnd() {
    std::string text = getText();
    size_t pos = cursor_.pos;
    while (pos < text.length() && text[pos] != '\n') pos++;
    moveCursor(pos);
}

void EditorCore::moveCursorLineStart() {
    std::string text = getText();
    size_t pos = cursor_.pos;
    while (pos > 0 && text[pos-1] != '\n') pos--;
    moveCursor(pos);
}

void EditorCore::moveCursorDocumentStart() {
    moveCursor(0);
}

void EditorCore::moveCursorDocumentEnd() {
    moveCursor(piece_table_.length());
}

void EditorCore::selectRange(size_t start, size_t end) {
    selection_.start = start;
    selection_.end = end;
    cursor_.pos = end;
    updateCursorLineCol();
}

void EditorCore::selectWord() {
    std::string text = getText();
    size_t start = cursor_.pos;
    size_t end = cursor_.pos;
    auto is_word_char = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    };

    while (start > 0 && is_word_char(text[start - 1])) start--;
    while (end < text.length() && is_word_char(text[end])) end++;

    selectRange(start, end);
}

void EditorCore::selectAll() {
    selectRange(0, piece_table_.length());
}

void EditorCore::clearSelection() {
    selection_.start = selection_.end = cursor_.pos;
}

void EditorCore::insert(const std::string& text) {
    size_t pos = hasSelection() ? selection_.min() : cursor_.pos;

    if (hasSelection()) {
        std::string deleted = piece_table_.getTextRange(selection_.min(), selection_.max());
        undo_stack_.push(Command{CommandType::DELETE, selection_.min(), deleted, deleted.length()});
        piece_table_.remove(selection_.min(), selection_.max());
        pos = selection_.min();
        clearSelection();
    }

    piece_table_.insert(pos, text);
    undo_stack_.push(Command{CommandType::INSERT, pos, text, text.length()});

    cursor_.pos = pos + text.length();
    updateCursorLineCol();
    updateWordCount();
}

void EditorCore::deleteSelection() {
    if (!hasSelection()) return;

    std::string deleted = piece_table_.getTextRange(selection_.min(), selection_.max());
    undo_stack_.push(Command{CommandType::DELETE, selection_.min(), deleted, deleted.length()});
    piece_table_.remove(selection_.min(), selection_.max());

    cursor_.pos = selection_.min();
    clearSelection();
    updateCursorLineCol();
    updateWordCount();
}

void EditorCore::deleteChar(int direction) {
    if (hasSelection()) {
        deleteSelection();
        return;
    }

    if (direction < 0) {  // Backspace
        if (cursor_.pos == 0) return;
        std::string text = getText();
        std::string deleted = text.substr(cursor_.pos - 1, 1);
        undo_stack_.push(Command{CommandType::DELETE, cursor_.pos - 1, deleted, 1});
        piece_table_.remove(cursor_.pos - 1, cursor_.pos);
        moveCursorRelative(-1);
    } else {  // Delete
        if (cursor_.pos >= piece_table_.length()) return;
        std::string text = getText();
        std::string deleted = text.substr(cursor_.pos, 1);
        undo_stack_.push(Command{CommandType::DELETE, cursor_.pos, deleted, 1});
        piece_table_.remove(cursor_.pos, cursor_.pos + 1);
    }
    updateWordCount();
}

void EditorCore::deleteWord(int direction) {
    if (hasSelection()) {
        deleteSelection();
        return;
    }

    size_t start = cursor_.pos;
    size_t end = cursor_.pos;

    std::string text = getText();
    auto is_word_char = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    };

    if (direction < 0) {  // Ctrl+Backspace
        if (start == 0) return;
        start--;
        while (start > 0 && std::isspace(static_cast<unsigned char>(text[start]))) start--;
        while (start > 0 && !is_word_char(text[start]) && !std::isspace(static_cast<unsigned char>(text[start]))) start--;
        while (start > 0 && is_word_char(text[start - 1])) start--;
    } else {  // Ctrl+Delete
        while (end < text.length() && is_word_char(text[end])) end++;
        while (end < text.length() && !is_word_char(text[end]) && !std::isspace(static_cast<unsigned char>(text[end]))) end++;
        while (end < text.length() && std::isspace(static_cast<unsigned char>(text[end]))) end++;
    }

    std::string deleted = piece_table_.getTextRange(start, end);
    undo_stack_.push(Command{CommandType::DELETE, start, deleted, deleted.length()});
    piece_table_.remove(start, end);

    cursor_.pos = start;
    updateCursorLineCol();
    updateWordCount();
}

void EditorCore::undo() {
    if (!canUndo()) return;
    Command cmd = undo_stack_.undo();

    if (cmd.type == CommandType::INSERT) {
        piece_table_.remove(cmd.position, cmd.position + cmd.length);
        cursor_.pos = cmd.position;
    } else if (cmd.type == CommandType::DELETE) {
        piece_table_.insert(cmd.position, cmd.text);
        cursor_.pos = cmd.position + cmd.length;
    }

    clearSelection();
    updateCursorLineCol();
    updateWordCount();
}

void EditorCore::redo() {
    if (!canRedo()) return;
    Command cmd = undo_stack_.redo();

    if (cmd.type == CommandType::INSERT) {
        piece_table_.insert(cmd.position, cmd.text);
        cursor_.pos = cmd.position + cmd.length;
    } else if (cmd.type == CommandType::DELETE) {
        piece_table_.remove(cmd.position, cmd.position + cmd.length);
        cursor_.pos = cmd.position;
    }

    clearSelection();
    updateCursorLineCol();
    updateWordCount();
}

std::vector<EditorCore::Match> EditorCore::find(const std::string& query) const {
    std::vector<Match> matches;
    if (query.empty()) return matches;

    std::string text = getText();
    size_t pos = 0;
    while ((pos = text.find(query, pos)) != std::string::npos) {
        matches.push_back({pos, pos + query.length()});
        pos += query.length();
    }
    return matches;
}

size_t EditorCore::findNext(const std::string& query, size_t from) const {
    if (query.empty()) return from;
    std::string text = getText();
    size_t pos = text.find(query, from);
    return (pos != std::string::npos) ? pos : from;
}

size_t EditorCore::findPrev(const std::string& query, size_t from) const {
    if (query.empty()) return from;
    std::string text = getText();
    size_t pos = text.rfind(query, from);
    return (pos != std::string::npos) ? pos : from;
}

std::string EditorCore::getTextAroundCursor(size_t chars_before, size_t chars_after) const {
    std::string text = getText();
    size_t start = (cursor_.pos > chars_before) ? cursor_.pos - chars_before : 0;
    size_t end = std::min(cursor_.pos + chars_after, text.length());
    return text.substr(start, end - start);
}

void EditorCore::updateCursorLineCol() {
    std::string text = getText();
    cursor_.line = 0;
    cursor_.col = 0;

    size_t pos = 0;
    while (pos < cursor_.pos && pos < text.length()) {
        if (text[pos] == '\n') {
            cursor_.line++;
            cursor_.col = 0;
        } else {
            cursor_.col++;
        }
        pos++;
    }

    line_count_ = 1;
    for (char c : text) {
        if (c == '\n') line_count_++;
    }
}

size_t EditorCore::calculateLineMoveTarget(int delta) const {
    if (delta == 0) return cursor_.pos;

    std::string text = getText();
    if (text.empty()) {
        return 0;
    }

    int64_t target_line = static_cast<int64_t>(cursor_.line) + delta;
    if (target_line < 0) target_line = 0;
    if (target_line >= static_cast<int64_t>(line_count_)) {
        target_line = static_cast<int64_t>(line_count_ - 1);
    }

    size_t line_start = 0;
    size_t line = 0;
    for (size_t i = 0; i < text.length() && line < static_cast<size_t>(target_line); ++i) {
        if (text[i] == '\n') {
            line++;
            line_start = i + 1;
        }
    }

    size_t line_end = line_start;
    while (line_end < text.length() && text[line_end] != '\n') {
        line_end++;
    }

    size_t target_col = std::min(cursor_.col, line_end - line_start);
    return line_start + target_col;
}

void EditorCore::updateWordCount() {
    std::string text = getText();
    word_count_ = 0;
    bool in_word = false;

    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (in_word) {
                word_count_++;
                in_word = false;
            }
        } else {
            in_word = true;
        }
    }
    if (in_word) word_count_++;
}
