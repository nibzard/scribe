#pragma once

#include <lvgl.h>
#include <string>
#include <vector>

// Custom TextView widget for efficient text rendering
// Renders only visible viewport, handles cursor and selection

class TextView {
public:
    TextView(lv_obj_t* parent);
    ~TextView();

    // LVGL object access
    lv_obj_t* obj() const { return obj_; }

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
    std::pair<size_t, size_t> getSelection() const {
        return {selection_start_, selection_end_};
    }

    // Viewport management
    void scrollToCursor();
    void scrollToLine(size_t line);

    // Configuration
    void setFont(const lv_font_t* font);
    void setLineHeight(int height);
    void setViewportSize(int width, int height);

    // Get line info
    size_t getLineCount() const { return line_cache_.size(); }
    size_t posToLine(size_t pos) const;
    size_t lineToPos(size_t line) const;

protected:
    // Line info for rendering
    struct LineInfo {
        size_t start_pos;
        size_t length;
        int y_offset;  // Y position in pixels
    };

    lv_obj_t* obj_;
    std::string text_;
    size_t cursor_pos_ = 0;
    size_t selection_start_ = 0;
    size_t selection_end_ = 0;

    // Rendering configuration
    const lv_font_t* font_ = &lv_font_montserrat_16;
    int line_height_ = 20;
    int viewport_width_ = 0;
    int viewport_height_ = 0;
    int char_width_ = 10;  // Approximate monospace width

    // Viewport state
    int scroll_y_ = 0;
    int visible_lines_ = 20;

    // Line wrap cache
    std::vector<LineInfo> line_cache_;

    // Helper methods
    void updateLineCache();
    int measureTextWidth(const char* text, size_t length) const;
    void invalidate();
    
    // Allow LVGL draw callback to access internals for rendering
    friend void text_view_draw_cb(lv_event_t* e);
};
