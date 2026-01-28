#include "text_view.h"
#include "../theme/theme.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "SCRIBE_TEXT_VIEW";

// Draw callback for LVGL
void text_view_draw_cb(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target_obj(e);
    lv_layer_t* layer = lv_event_get_layer(e);
    if (!layer) {
        return;
    }

    // Get user data pointer to TextView instance
    TextView* tv = static_cast<TextView*>(lv_obj_get_user_data(obj));
    if (!tv) {
        return;
    }

    // Get object dimensions
    lv_area_t obj_coords;
    lv_obj_get_coords(obj, &obj_coords);

    int x = obj_coords.x1;
    int y = obj_coords.y1 - tv->scroll_y_;
    int width = lv_area_get_width(&obj_coords);

    // Clip drawing area to visible region
    lv_area_t clip_area;
    lv_area_copy(&clip_area, &obj_coords);

    const Theme::Colors& colors = Theme::getColors();

    // Draw background
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.base.layer = layer;
    rect_dsc.bg_color = colors.bg;
    rect_dsc.border_width = 0;
    lv_draw_rect(layer, &rect_dsc, &obj_coords);

    // Draw text line by line
    const std::string& text = tv->text_;
    const auto& line_cache = tv->line_cache_;

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.base.layer = layer;
    label_dsc.font = tv->font_;
    label_dsc.color = colors.text;
    label_dsc.flag = LV_TEXT_FLAG_EXPAND;

    // Determine selection range
    size_t sel_start = tv->selection_start_;
    size_t sel_end = tv->selection_end_;
    if (sel_start > sel_end) {
        std::swap(sel_start, sel_end);
    }

    for (const auto& line : line_cache) {
        int line_y = y + line.y_offset;

        // Skip if outside visible area
        if (line_y + tv->line_height_ < clip_area.y1 ||
            line_y > clip_area.y2) {
            continue;
        }

        // Draw selection background for this line if selected
        if (sel_start < sel_end) {
            size_t line_end = line.start_pos + line.length;
            // Check if selection overlaps with this line
            if (sel_start < line_end && sel_end > line.start_pos) {
                size_t sel_line_start = std::max(sel_start, line.start_pos);
                size_t sel_line_end = std::min(sel_end, line_end);

                // Calculate selection coordinates
                int sel_x = 0;
                int sel_width = 0;
                if (tv->use_snapshot_) {
                    std::string prefix = tv->getTextRange(line.start_pos, sel_line_start);
                    std::string selection = tv->getTextRange(sel_line_start, sel_line_end);
                    sel_x = x + tv->measureTextWidth(prefix.c_str(), prefix.size());
                    sel_width = tv->measureTextWidth(selection.c_str(), selection.size());
                } else {
                    sel_x = x + tv->measureTextWidth(
                        text.c_str() + line.start_pos,
                        sel_line_start - line.start_pos
                    );
                    sel_width = tv->measureTextWidth(
                        text.c_str() + sel_line_start,
                        sel_line_end - sel_line_start
                    );
                }

                lv_area_t sel_area;
                sel_area.x1 = sel_x;
                sel_area.y1 = line_y;
                sel_area.x2 = sel_x + sel_width - 1;
                sel_area.y2 = line_y + tv->line_height_ - 1;

                lv_draw_rect_dsc_t sel_dsc;
                lv_draw_rect_dsc_init(&sel_dsc);
                sel_dsc.base.layer = layer;
                sel_dsc.bg_color = colors.selection;
                sel_dsc.bg_opa = LV_OPA_50;
                sel_dsc.border_width = 0;
                lv_draw_rect(layer, &sel_dsc, &sel_area);
            }
        }

        // Draw text for this line
        lv_area_t text_area;
        text_area.x1 = x;
        text_area.y1 = line_y;
        text_area.x2 = x + width - 1;
        text_area.y2 = line_y + tv->line_height_ - 1;

        std::string line_text_buffer;
        const char* line_text = nullptr;
        size_t line_len = line.length;
        if (tv->use_snapshot_) {
            line_text_buffer = tv->getTextRange(line.start_pos, line.start_pos + line.length);
            line_text = line_text_buffer.c_str();
            line_len = line_text_buffer.size();
        } else {
            line_text = text.c_str() + line.start_pos;
        }

        // Remove trailing newline for display
        if (line_len > 0 && line_text[line_len - 1] == '\n') {
            line_len--;
        }

        label_dsc.text = line_text;
        label_dsc.text_length = line_len;
        lv_draw_label(layer, &label_dsc, &text_area);
    }

    // Draw cursor
    size_t cursor_line = tv->posToLine(tv->cursor_pos_);
    if (cursor_line < line_cache.size()) {
        const auto& line = line_cache[cursor_line];
        int cursor_y = y + line.y_offset;
        size_t pos_in_line = tv->cursor_pos_ - line.start_pos;
        int cursor_x = 0;
        if (tv->use_snapshot_) {
            std::string prefix = tv->getTextRange(line.start_pos, line.start_pos + pos_in_line);
            cursor_x = x + tv->measureTextWidth(prefix.c_str(), prefix.size());
        } else {
            cursor_x = x + tv->measureTextWidth(
                text.c_str() + line.start_pos,
                pos_in_line
            );
        }

        lv_area_t cursor_area;
        cursor_area.x1 = cursor_x;
        cursor_area.y1 = cursor_y;
        cursor_area.x2 = cursor_x + 2;
        cursor_area.y2 = cursor_y + tv->line_height_ - 1;

        lv_draw_rect_dsc_t cursor_dsc;
        lv_draw_rect_dsc_init(&cursor_dsc);
        cursor_dsc.base.layer = layer;
        cursor_dsc.bg_color = colors.text;
        cursor_dsc.bg_opa = LV_OPA_COVER;
        cursor_dsc.border_width = 0;
        lv_draw_rect(layer, &cursor_dsc, &cursor_area);
    }
}

TextView::TextView(lv_obj_t* parent) {
    // Create LVGL object
    obj_ = lv_obj_create(parent);
    if (!obj_) {
        ESP_LOGE(TAG, "Failed to create TextView object");
        return;
    }

    // Set user data pointer for draw callback
    lv_obj_set_user_data(obj_, this);

    // Configure object
    lv_obj_set_size(obj_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(obj_, 0, 0);
    lv_obj_set_scrollbar_mode(obj_, LV_SCROLLBAR_MODE_OFF);
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(obj_, colors.bg, 0);
    lv_obj_set_style_bg_opa(obj_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj_, 0, 0);
    lv_obj_set_style_pad_all(obj_, 10, 0);

    // Add draw callback
    lv_obj_add_event_cb(obj_, text_view_draw_cb, LV_EVENT_DRAW_MAIN, nullptr);

    // Get initial viewport size
    lv_obj_update_layout(obj_);
    lv_area_t coords;
    lv_obj_get_coords(obj_, &coords);
    int width = lv_area_get_width(&coords);
    int height = lv_area_get_height(&coords);
    viewport_width_ = width > 20 ? (width - 20) : 1;  // Account for padding
    viewport_height_ = height > 20 ? (height - 20) : 1;
    visible_lines_ = viewport_height_ / line_height_;
    if (visible_lines_ < 1) {
        visible_lines_ = 1;
    }

    ESP_LOGI(TAG, "TextView created: viewport %dx%d, %d lines",
             viewport_width_, viewport_height_, visible_lines_);
}

TextView::~TextView() {
    if (obj_) {
        lv_obj_delete(obj_);
        obj_ = nullptr;
    }
}

void TextView::setText(const std::string& text) {
    use_snapshot_ = false;
    text_ = text;
    total_length_ = text_.length();
    updateLineCache();
    invalidate();
}

void TextView::setSnapshot(const PieceTableSnapshot& snapshot) {
    snapshot_ = snapshot;
    use_snapshot_ = true;
    text_.clear();
    total_length_ = snapshot_.total_length;
    updateLineCache();
    invalidate();
}

void TextView::setCursor(size_t pos) {
    size_t length = use_snapshot_ ? total_length_ : text_.length();
    if (pos > length) {
        pos = length;
    }
    cursor_pos_ = pos;
    scrollToCursor();
    invalidate();
}

void TextView::setSelection(size_t start, size_t end) {
    selection_start_ = start;
    selection_end_ = end;
    invalidate();
}

void TextView::clearSelection() {
    selection_start_ = selection_end_ = 0;
    invalidate();
}

void TextView::scrollToCursor() {
    size_t line = posToLine(cursor_pos_);
    if (line < line_cache_.size()) {
        int cursor_y = line_cache_[line].y_offset;

        // Scroll if cursor is above visible area
        if (cursor_y < scroll_y_) {
            scroll_y_ = cursor_y;
        }
        // Scroll if cursor is below visible area
        else if (cursor_y + line_height_ > scroll_y_ + viewport_height_) {
            scroll_y_ = cursor_y + line_height_ - viewport_height_;
        }
    }
}

void TextView::scrollToLine(size_t line) {
    if (line < line_cache_.size()) {
        scroll_y_ = line_cache_[line].y_offset;
        invalidate();
    }
}

void TextView::setFont(const lv_font_t* font) {
    if (font) {
        font_ = font;
        // Approximate width using 'M' glyph if available
        char_width_ = lv_font_get_glyph_width(font_, 'M', 0);
        if (char_width_ <= 0) char_width_ = 10;
        line_height_ = font_->line_height;
        visible_lines_ = viewport_height_ / line_height_;
        if (visible_lines_ < 1) {
            visible_lines_ = 1;
        }
        updateLineCache();
        invalidate();
    }
}

void TextView::setLineHeight(int height) {
    line_height_ = height;
    visible_lines_ = viewport_height_ / line_height_;
    if (visible_lines_ < 1) {
        visible_lines_ = 1;
    }
    updateLineCache();
    invalidate();
}

void TextView::setViewportSize(int width, int height) {
    viewport_width_ = width > 0 ? width : 1;
    viewport_height_ = height > 0 ? height : 1;
    visible_lines_ = viewport_height_ / line_height_;
    if (visible_lines_ < 1) {
        visible_lines_ = 1;
    }
    updateLineCache();
}

void TextView::applyTheme() {
    if (!obj_) {
        return;
    }
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(obj_, colors.bg, 0);
    lv_obj_set_style_bg_opa(obj_, LV_OPA_COVER, 0);
    invalidate();
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

size_t TextView::lineToPos(size_t line) const {
    if (line < line_cache_.size()) {
        return line_cache_[line].start_pos;
    }
    return use_snapshot_ ? total_length_ : text_.length();
}

void TextView::updateLineCache() {
    line_cache_.clear();

    if (use_snapshot_) {
        updateLineCacheFromSnapshot();
        return;
    }

    if (text_.empty()) {
        return;
    }

    size_t line_start = 0;
    int y_offset = 0;
    int line_width = 0;

    for (size_t i = 0; i <= text_.length(); i++) {
        bool is_newline = (i < text_.length() && text_[i] == '\n');

        // Check if we need to wrap or hit newline
        if (is_newline || line_width >= viewport_width_) {
            line_cache_.push_back({line_start, i - line_start, y_offset});

            if (is_newline) {
                line_start = i + 1;
            } else {
                line_start = i;
            }

            y_offset += line_height_;
            line_width = 0;

            // Skip the newline character
            if (is_newline) {
                continue;
            }
        }

        if (i < text_.length()) {
            char c = text_[i];
            if (c == '\t') {
                line_width += char_width_ * 4;  // Tab = 4 spaces
            } else if (c >= 32 && c < 127) {
                line_width += char_width_;
            }
            // Non-printable chars take minimal space
        }
    }

    // Don't forget the last line if it doesn't end with newline
    if (line_start < text_.length()) {
        line_cache_.push_back({line_start, text_.length() - line_start, y_offset});
    }

    ESP_LOGD(TAG, "Line cache updated: %zu lines, total height %d",
             line_cache_.size(), y_offset + line_height_);
}

void TextView::updateLineCacheFromSnapshot() {
    if (!snapshot_.original_buffer || !snapshot_.add_buffer) {
        return;
    }

    if (total_length_ == 0 || snapshot_.pieces.empty()) {
        return;
    }

    size_t line_start = 0;
    size_t pos = 0;
    int y_offset = 0;
    int line_width = 0;

    for (const auto& piece : snapshot_.pieces) {
        const std::string& buffer = (piece.type == Piece::Type::ORIGINAL)
            ? *snapshot_.original_buffer
            : *snapshot_.add_buffer;

        for (size_t i = 0; i < piece.length; ++i) {
            if (pos >= total_length_) {
                break;
            }

            char c = buffer[piece.start + i];
            bool is_newline = (c == '\n');

            if (is_newline || line_width >= viewport_width_) {
                line_cache_.push_back({line_start, pos - line_start, y_offset});
                line_start = is_newline ? pos + 1 : pos;
                y_offset += line_height_;
                line_width = 0;
                if (is_newline) {
                    pos++;
                    continue;
                }
            }

            if (c == '\t') {
                line_width += char_width_ * 4;
            } else if (c >= 32 && c < 127) {
                line_width += char_width_;
            }

            pos++;
        }
    }

    if (line_start < total_length_) {
        line_cache_.push_back({line_start, total_length_ - line_start, y_offset});
    }

    ESP_LOGD(TAG, "Line cache updated: %zu lines, total height %d",
             line_cache_.size(), y_offset + line_height_);
}

std::string TextView::getTextRange(size_t start, size_t end) const {
    if (start >= end) {
        return "";
    }

    if (!use_snapshot_) {
        if (end > text_.length()) {
            end = text_.length();
        }
        return text_.substr(start, end - start);
    }

    if (!snapshot_.original_buffer || !snapshot_.add_buffer) {
        return "";
    }

    if (end > total_length_) {
        end = total_length_;
    }

    std::string result;
    result.reserve(end - start);

    size_t pos = 0;
    for (const auto& piece : snapshot_.pieces) {
        size_t piece_start_pos = pos;
        size_t piece_end_pos = pos + piece.length;
        if (end <= piece_start_pos) {
            break;
        }
        if (start >= piece_end_pos) {
            pos = piece_end_pos;
            continue;
        }

        size_t local_start = (start > piece_start_pos) ? (start - piece_start_pos) : 0;
        size_t local_end = (end < piece_end_pos) ? (end - piece_start_pos) : piece.length;

        const std::string& buffer = (piece.type == Piece::Type::ORIGINAL)
            ? *snapshot_.original_buffer
            : *snapshot_.add_buffer;
        result.append(buffer.substr(piece.start + local_start, local_end - local_start));

        pos = piece_end_pos;
    }

    return result;
}

int TextView::measureTextWidth(const char* text, size_t length) const {
    if (!text || length == 0) {
        return 0;
    }

    int width = 0;
    for (size_t i = 0; i < length && text[i] != '\0'; i++) {
        char c = text[i];
        if (c == '\t') {
            width += char_width_ * 4;
        } else if (c >= 32 && c < 127) {
            width += char_width_;
        }
    }
    return width;
}

void TextView::invalidate() {
    if (obj_) {
        lv_obj_invalidate(obj_);
    }
}
