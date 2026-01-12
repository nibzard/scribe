#pragma once

#include <lvgl.h>
#include <string>
#include <vector>

// Custom TextView widget for efficient text rendering
// Renders only visible viewport, handles cursor and selection

class TextView : public lv_obj {
public:
    TextView(lv_obj_t* parent);

    // Set text content
    void setText(const std::string& text);

    // Get text content
    std::string getText() const { return text_; }

    // Cursor position
    void setCursor(size_t pos);
    size_t getCursor() const { return cursor_pos_; }

    // Selection
    void setSelection(size_t start, size_t end);
    void clearSelection();

    // Viewport scroll
    void scrollToCursor();

protected:
    // LVGL draw callback
    void draw_cb(lv_draw_ctx_t* draw_ctx) override;

private:
    std::string text_;
    size_t cursor_pos_ = 0;
    size_t selection_start_ = 0;
    size_t selection_end_ = 0;

    // Viewport
    int viewport_y_ = 0;
    int line_height_ = 20;
    int visible_lines_ = 20;

    // Line wrap cache
    struct LineInfo {
        size_t start_pos;
        size_t length;
    };
    std::vector<LineInfo> line_cache_;

    void updateLineCache();
    size_t posToLine(size_t pos) const;
};
