#include "text_view.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_TEXT_VIEW";

TextView::TextView(lv_obj_t* parent) : lv_obj() {
    // Create LVGL object
    // TODO: Implement custom LVGL widget class
}

void TextView::setText(const std::string& text) {
    text_ = text;
    updateLineCache();
    lv_obj_invalidate(this);
}

void TextView::setCursor(size_t pos) {
    if (pos > text_.length()) pos = text_.length();
    cursor_pos_ = pos;
    scrollToCursor();
}

void TextView::setSelection(size_t start, size_t end) {
    selection_start_ = start;
    selection_end_ = end;
    lv_obj_invalidate(this);
}

void TextView::clearSelection() {
    selection_start_ = selection_end_ = 0;
    lv_obj_invalidate(this);
}

void TextView::scrollToCursor() {
    // TODO: Implement scrolling to keep cursor in view
}

void TextView::updateLineCache() {
    line_cache_.clear();

    size_t pos = 0;
    size_t line_start = 0;
    int line_width = 0;

    for (size_t i = 0; i < text_.length(); i++) {
        if (text_[i] == '\n') {
            line_cache_.push_back({line_start, i - line_start + 1});
            line_start = i + 1;
            line_width = 0;
        } else {
            line_width++;
            // TODO: Implement proper line wrapping based on viewport width
        }
    }

    // Don't forget the last line
    if (line_start < text_.length()) {
        line_cache_.push_back({line_start, text_.length() - line_start});
    }
}

size_t TextView::posToLine(size_t pos) const {
    for (size_t i = 0; i < line_cache_.size(); i++) {
        if (pos >= line_cache_[i].start_pos &&
            (i == line_cache_.size() - 1 || pos < line_cache_[i + 1].start_pos)) {
            return i;
        }
    }
    return 0;
}
