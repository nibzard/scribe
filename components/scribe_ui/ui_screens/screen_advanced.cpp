#include "screen_advanced.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_ADVANCED";

ScreenAdvanced::ScreenAdvanced() : screen_(nullptr) {
}

ScreenAdvanced::~ScreenAdvanced() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenAdvanced::init() {
    ESP_LOGI(TAG, "Initializing advanced settings screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenAdvanced::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("settings.advanced"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_18, 0);

    // Settings list
    settings_list_ = lv_list_create(screen_);
    lv_obj_set_size(settings_list_, 350, 350);
    lv_obj_align(settings_list_, LV_ALIGN_TOP_MID, 0, 70);
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(settings_list_, colors.fg, 0);
    lv_obj_set_style_bg_opa(settings_list_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(settings_list_, colors.border, 0);
    lv_obj_set_style_border_width(settings_list_, 1, 0);

    buttons_.clear();
    item_keys_.clear();

    auto addItem = [&](const char* label, const char* key, const char* symbol) {
        lv_obj_t* btn = lv_list_add_button(settings_list_, symbol, label);
        lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
        buttons_.push_back(btn);
        item_keys_.push_back(key);
    };

    Strings& strings = Strings::getInstance();
    addItem(strings.get("settings.wifi"), "wifi", LV_SYMBOL_WIFI);
    addItem(strings.get("settings.backup"), "backup", LV_SYMBOL_UPLOAD);
    addItem(strings.get("settings.ai"), "ai", LV_SYMBOL_SHUFFLE);
    addItem(strings.get("settings.diagnostics"), "diagnostics", LV_SYMBOL_SETTINGS);
    addItem(strings.get("settings.back"), "back", LV_SYMBOL_LEFT);

    selected_index_ = 0;
    updateSelection();
}

void ScreenAdvanced::show() {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        const Theme::Colors& colors = Theme::getColors();
        if (settings_list_) {
            lv_obj_set_style_bg_color(settings_list_, colors.fg, 0);
            lv_obj_set_style_bg_opa(settings_list_, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(settings_list_, colors.border, 0);
            lv_obj_set_style_border_width(settings_list_, 1, 0);
        }
        for (auto* btn : buttons_) {
            if (!btn) continue;
            lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
        }
        lv_screen_load(screen_);
        updateSelection();
    }
}

void ScreenAdvanced::hide() {
    if (back_cb_) {
        back_cb_();
    }
}

void ScreenAdvanced::moveSelection(int delta) {
    int count = static_cast<int>(buttons_.size());
    if (count <= 0) return;

    selected_index_ += delta;
    if (selected_index_ < 0) selected_index_ = count - 1;
    if (selected_index_ >= count) selected_index_ = 0;

    updateSelection();
}

void ScreenAdvanced::selectCurrent() {
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(item_keys_.size())) {
        return;
    }
    const std::string& key = item_keys_[selected_index_];
    if (key == "back") {
        if (back_cb_) back_cb_();
        return;
    }
    if (navigate_cb_) {
        navigate_cb_(key.c_str());
    }
}

void ScreenAdvanced::updateSelection() {
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (static_cast<int>(i) == selected_index_) {
            lv_obj_add_state(buttons_[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(buttons_[i], LV_STATE_CHECKED);
        }
    }
    lv_obj_invalidate(settings_list_);
}
