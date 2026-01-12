#include "screen_find.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_FIND";

ScreenFind::ScreenFind() : overlay_(nullptr) {
}

ScreenFind::~ScreenFind() {
    if (overlay_) {
        lv_obj_del(overlay_);
    }
}

void ScreenFind::init() {
    ESP_LOGI(TAG, "Initializing find bar");
    createWidgets();
}

void ScreenFind::createWidgets() {
    // Create overlay that appears at bottom of screen
    overlay_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay_, LV_HOR_RES, 80);
    lv_obj_align(overlay_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(overlay_, lv_color_white(), 0);
    lv_obj_set_style_border_color(overlay_, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_border_width(overlay_, 1, 0);
    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);

    // Find label
    label_ = lv_label_create(overlay_);
    lv_label_set_text(label_, "Find");
    lv_obj_align(label_, LV_ALIGN_LEFT_MID, 20, 0);

    // Search input
    search_input_ = lv_textarea_create(overlay_);
    lv_obj_set_size(search_input_, 300, 40);
    lv_obj_align(search_input_, LV_ALIGN_LEFT_MID, 70, 0);
    lv_textarea_set_placeholder_text(search_input_, "Type to search\u2026");
    lv_textarea_set_one_line(search_input_, true);

    // Match count label
    match_label_ = lv_label_create(overlay_);
    lv_label_set_text(match_label_, "");
    lv_obj_align(match_label_, LV_ALIGN_RIGHT_MID, -100, 0);

    // Hint label
    hint_label_ = lv_label_create(overlay_);
    lv_label_set_text(hint_label_, "Next (Enter)  Previous (Shift+Enter)  Close (Esc)");
    lv_obj_align(hint_label_, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_set_style_text_font(hint_label_, &lv_font_montserrat_12, 0);
}

void ScreenFind::show() {
    if (overlay_) {
        lv_obj_clear_flag(overlay_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(search_input_, LV_STATE_FOCUSED);
    }
}

void ScreenFind::hide() {
    if (overlay_) {
        lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);
    }
    if (close_cb_) {
        close_cb_();
    }
}

void ScreenFind::setQuery(const std::string& query) {
    if (search_input_) {
        lv_textarea_set_text(search_input_, query.c_str());
    }
}

std::string ScreenFind::getQuery() const {
    if (search_input_) {
        const char* text = lv_textarea_get_text(search_input_);
        return std::string(text ? text : "");
    }
    return "";
}

void ScreenFind::appendQueryChar(char c) {
    if (!search_input_) return;
    char buf[2] = {c, 0};
    lv_textarea_add_text(search_input_, buf);
}

void ScreenFind::backspaceQuery() {
    if (search_input_) {
        lv_textarea_del_char(search_input_);
    }
}

void ScreenFind::showMatch(int current, int total) {
    if (match_label_) {
        lv_label_set_text_fmt(match_label_, "%d/%d matches", current, total);
    }
}

void ScreenFind::showNoResults() {
    if (match_label_) {
        lv_label_set_text(match_label_, "No matches.");
    }
}
