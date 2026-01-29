#include "text_view.h"
#include "../theme/theme.h"
#include "misc/lv_text_private.h"
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

    int x = obj_coords.x1 + tv->inset_left_;
    int y = obj_coords.y1 + tv->inset_top_ - tv->scroll_y_;
    int width = lv_area_get_width(&obj_coords) - tv->inset_left_ - tv->inset_right_;
    if (width < 1) {
        width = 1;
    }

    // Clip drawing area to visible region
    lv_area_t clip_area;
    lv_area_copy(&clip_area, &obj_coords);
    clip_area.x1 += tv->inset_left_;
    clip_area.y1 += tv->inset_top_;
    clip_area.x2 -= tv->inset_right_;
    clip_area.y2 -= tv->inset_bottom_;
    if (clip_area.x2 < clip_area.x1) {
        clip_area.x2 = clip_area.x1;
    }
    if (clip_area.y2 < clip_area.y1) {
        clip_area.y2 = clip_area.y1;
    }

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
    const char* snapshot_text = (tv->use_snapshot_ && !tv->snapshot_text_cache_.empty())
        ? tv->snapshot_text_cache_.c_str()
        : nullptr;

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
                    if (snapshot_text) {
                        sel_x = x + tv->measureTextWidth(
                            snapshot_text + line.start_pos,
                            sel_line_start - line.start_pos
                        );
                        sel_width = tv->measureTextWidth(
                            snapshot_text + sel_line_start,
                            sel_line_end - sel_line_start
                        );
                    }
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

        const char* line_text = nullptr;
        size_t line_len = line.length;
        if (tv->use_snapshot_) {
            if (snapshot_text) {
                line_text = snapshot_text + line.start_pos;
            } else {
                line_len = 0;
            }
        } else {
            line_text = text.c_str() + line.start_pos;
        }

        if (!line_text || line_len == 0) {
            continue;
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
            if (snapshot_text) {
                cursor_x = x + tv->measureTextWidth(
                    snapshot_text + line.start_pos,
                    pos_in_line
                );
            }
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
    lv_obj_set_style_pad_all(obj_, 0, 0);

    // Add draw callback
    lv_obj_add_event_cb(obj_, text_view_draw_cb, LV_EVENT_DRAW_MAIN, nullptr);

    // Get initial viewport size
    lv_obj_update_layout(obj_);
    lv_area_t coords;
    lv_obj_get_coords(obj_, &coords);
    int width = lv_area_get_width(&coords);
    int height = lv_area_get_height(&coords);
    viewport_width_ = width > 0 ? width : 1;
    viewport_height_ = height > 0 ? height : 1;
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
    snapshot_text_cache_.clear();
    total_length_ = text_.length();
    updateLineCache();
    invalidate();
}

void TextView::setSnapshot(const PieceTableSnapshot& snapshot) {
    snapshot_ = snapshot;
    use_snapshot_ = true;
    text_.clear();
    snapshot_text_cache_.clear();
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

void TextView::setContentInsets(int left, int top, int right, int bottom) {
    inset_left_ = left > 0 ? left : 0;
    inset_top_ = top > 0 ? top : 0;
    inset_right_ = right > 0 ? right : 0;
    inset_bottom_ = bottom > 0 ? bottom : 0;
    invalidate();
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

    lv_text_attributes_t attrs;
    lv_text_attributes_init(&attrs);
    attrs.letter_space = 0;
    attrs.line_space = 0;
    attrs.max_width = viewport_width_;
    attrs.text_flags = LV_TEXT_FLAG_BREAK_ALL;

    size_t index = 0;
    int y_offset = 0;
    const size_t text_len = text_.length();
    while (index < text_len) {
        uint32_t remaining = static_cast<uint32_t>(text_len - index);
        uint32_t line_bytes = lv_text_get_next_line(text_.c_str() + index, remaining, font_, nullptr, &attrs);
        if (line_bytes == 0) {
            break;
        }
        size_t line_len = line_bytes;
        char last = text_[index + line_bytes - 1];
        if (last == '\n' || last == '\r') {
            if (line_len > 0) {
                line_len -= 1;
            }
        }
        line_cache_.push_back({index, line_len, y_offset});
        y_offset += line_height_;
        index += line_bytes;
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

    snapshot_text_cache_ = getTextRange(0, total_length_);
    if (snapshot_text_cache_.empty()) {
        return;
    }

    lv_text_attributes_t attrs;
    lv_text_attributes_init(&attrs);
    attrs.letter_space = 0;
    attrs.line_space = 0;
    attrs.max_width = viewport_width_;
    attrs.text_flags = LV_TEXT_FLAG_BREAK_ALL;

    size_t index = 0;
    int y_offset = 0;
    const size_t text_len = snapshot_text_cache_.length();
    while (index < text_len) {
        uint32_t remaining = static_cast<uint32_t>(text_len - index);
        uint32_t line_bytes = lv_text_get_next_line(snapshot_text_cache_.c_str() + index, remaining, font_, nullptr, &attrs);
        if (line_bytes == 0) {
            break;
        }
        size_t line_len = line_bytes;
        char last = snapshot_text_cache_[index + line_bytes - 1];
        if (last == '\n' || last == '\r') {
            if (line_len > 0) {
                line_len -= 1;
            }
        }
        line_cache_.push_back({index, line_len, y_offset});
        y_offset += line_height_;
        index += line_bytes;
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
    if (!font_) {
        return 0;
    }

    lv_text_attributes_t attrs;
    lv_text_attributes_init(&attrs);
    attrs.letter_space = 0;
    attrs.line_space = 0;
    attrs.max_width = viewport_width_;
    attrs.text_flags = LV_TEXT_FLAG_NONE;

    const char* tab = static_cast<const char*>(memchr(text, '\t', length));
    if (!tab) {
        return lv_text_get_width(text, static_cast<uint32_t>(length), font_, &attrs);
    }

    int width = 0;
    int space_w = lv_font_get_glyph_width(font_, ' ', '\0');
    size_t offset = 0;
    while (offset < length) {
        const char* next_tab = static_cast<const char*>(memchr(text + offset, '\t', length - offset));
        size_t chunk_len = next_tab ? static_cast<size_t>(next_tab - (text + offset)) : (length - offset);
        if (chunk_len > 0) {
            width += lv_text_get_width(text + offset, static_cast<uint32_t>(chunk_len), font_, &attrs);
        }
        if (next_tab) {
            width += space_w * 4;
            offset += chunk_len + 1;
        } else {
            break;
        }
    }

    return width;
}

void TextView::invalidate() {
    if (obj_) {
        lv_obj_invalidate(obj_);
    }
}
