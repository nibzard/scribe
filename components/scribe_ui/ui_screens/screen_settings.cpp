#include "screen_settings.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>
#include <cstdio>

static const char* TAG = "SCRIBE_SCREEN_SETTINGS";

static const char* themeLabel(const std::string& theme_id) {
    Strings& strings = Strings::getInstance();
    const char* key = Theme::getThemeLabelKey(theme_id.c_str());
    return strings.get(key);
}

static const char* editorFontLabel(const std::string& font_id) {
    Strings& strings = Strings::getInstance();
    const char* key = Theme::getEditorFontLabelKey(font_id.c_str());
    return strings.get(key);
}

static std::string fontSizeLabel(int size) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d px", size);
    return std::string(buf);
}

static std::string uiScaleLabel(int percent) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    return std::string(buf);
}

static const char* marginLabel(int value) {
    Strings& strings = Strings::getInstance();
    switch (value) {
        case 0: return strings.get("settings.margin_small");
        case 1: return strings.get("settings.margin_medium");
        case 2: return strings.get("settings.margin_large");
        default: return strings.get("settings.margin_medium");
    }
}

static std::string brightnessLabel(int value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", value);
    return std::string(buf);
}

static const char* autoSleepLabel(int value) {
    Strings& strings = Strings::getInstance();
    switch (value) {
        case 0: return strings.get("settings.auto_sleep_off");
        case 1: return strings.get("settings.auto_sleep_5");
        case 2: return strings.get("settings.auto_sleep_15");
        case 3: return strings.get("settings.auto_sleep_30");
        default: return strings.get("settings.auto_sleep_15");
    }
}

static const char* orientationLabel(int value) {
    Strings& strings = Strings::getInstance();
    switch (value) {
        case 0: return strings.get("settings.orientation_auto");
        case 1: return strings.get("settings.orientation_landscape");
        case 2: return strings.get("settings.orientation_portrait");
        case 3: return strings.get("settings.orientation_landscape_inverted");
        case 4: return strings.get("settings.orientation_portrait_inverted");
        default: return strings.get("settings.orientation_auto");
    }
}

static const char* keyboardLayoutLabel(int value) {
    switch (value) {
        case 0: return "US";
        case 1: return "UK";
        case 2: return "DE";
        case 3: return "FR";
        case 4: return "HR";
        default: return "US";
    }
}

ScreenSettings::ScreenSettings() : screen_(nullptr), selected_index_(0), in_advanced_(false) {
}

ScreenSettings::~ScreenSettings() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenSettings::init() {
    ESP_LOGI(TAG, "Initializing settings screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenSettings::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("settings.title"));

    // Settings list
    settings_list_ = lv_list_create(screen_);
    applyTheme();
}

void ScreenSettings::setSettings(const AppSettings& settings) {
    settings_ = settings;
    rebuildList();
}

void ScreenSettings::rebuildList() {
    if (!settings_list_) return;

    lv_obj_clean(settings_list_);
    buttons_.clear();
    value_labels_.clear();
    item_keys_.clear();

    applyTheme();
    const Theme::Colors& colors = Theme::getColors();
    auto addItem = [&](const char* label, const char* value, const char* key, const char* symbol) -> lv_obj_t* {
        lv_obj_t* btn = lv_list_add_button(settings_list_, symbol, label);
        lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
        buttons_.push_back(btn);
        item_keys_.push_back(key);

        lv_obj_t* value_label = nullptr;
        if (value) {
            value_label = lv_label_create(btn);
            lv_label_set_text(value_label, value);
            lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, -Theme::scalePx(10), 0);
        }
        value_labels_.push_back(value_label);
        return btn;
    };

    Strings& strings = Strings::getInstance();
    lv_obj_t* theme_btn = addItem(strings.get("settings.theme"), themeLabel(settings_.theme_id),
                                  "theme", LV_SYMBOL_IMAGE);
    if (theme_btn) {
        lv_obj_t* value_label = value_labels_.empty() ? nullptr : value_labels_.back();
        if (value_label) {
            lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, -Theme::scalePx(90), 0);
        }

        lv_obj_t* swatch_row = lv_obj_create(theme_btn);
        lv_obj_set_size(swatch_row, Theme::scalePx(70), Theme::scalePx(14));
        lv_obj_clear_flag(swatch_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(swatch_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(swatch_row, 0, 0);
        lv_obj_align(swatch_row, LV_ALIGN_RIGHT_MID, -Theme::scalePx(10), 0);

        const lv_color_t swatches[] = {colors.bg, colors.fg, colors.accent, colors.selection};
        for (size_t i = 0; i < sizeof(swatches) / sizeof(swatches[0]); ++i) {
            lv_obj_t* swatch = lv_obj_create(swatch_row);
            lv_obj_set_size(swatch, Theme::scalePx(12), Theme::scalePx(12));
            lv_obj_clear_flag(swatch, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(swatch, swatches[i], 0);
            lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(swatch, colors.border, 0);
            lv_obj_set_style_border_width(swatch, 1, 0);
            lv_obj_align(swatch, LV_ALIGN_LEFT_MID, Theme::scalePx(static_cast<int>(i) * 16), 0);
        }
    }
    addItem(strings.get("settings.orientation"), orientationLabel(settings_.display_orientation),
            "display_orientation", LV_SYMBOL_LOOP);
    std::string font_label = fontSizeLabel(settings_.font_size);
    addItem(strings.get("settings.font_size"), font_label.c_str(), "font_size", LV_SYMBOL_EDIT);
    std::string scale_label = uiScaleLabel(settings_.ui_scale);
    addItem(strings.get("settings.ui_scale"), scale_label.c_str(), "ui_scale", LV_SYMBOL_PLUS);
    addItem(strings.get("settings.editor_font"), editorFontLabel(settings_.editor_font_id),
            "editor_font", LV_SYMBOL_EDIT);
    addItem(strings.get("settings.margin"), marginLabel(settings_.editor_margin), "editor_margin", LV_SYMBOL_LIST);
    addItem(strings.get("settings.keyboard_layout"), keyboardLayoutLabel(settings_.keyboard_layout), "keyboard_layout", LV_SYMBOL_KEYBOARD);
    addItem(strings.get("settings.auto_sleep"), autoSleepLabel(settings_.auto_sleep), "auto_sleep", LV_SYMBOL_GPS);
    std::string brightness_label = brightnessLabel(settings_.brightness);
    addItem(strings.get("settings.brightness"), brightness_label.c_str(), "brightness", LV_SYMBOL_EYE_OPEN);
    addItem(strings.get("settings.advanced"), nullptr, "advanced", LV_SYMBOL_SETTINGS);
    addItem(strings.get("settings.back"), nullptr, "back", LV_SYMBOL_LEFT);

    if (selected_index_ >= static_cast<int>(buttons_.size())) {
        selected_index_ = 0;
    }
    updateSelection();
}

void ScreenSettings::show() {
    if (screen_) {
        applyTheme();
        lv_screen_load(screen_);
        updateSelection();
    }
}

void ScreenSettings::hide() {
    if (picker_active_) {
        hidePicker();
    }
    if (close_cb_) {
        close_cb_();
    }
}

void ScreenSettings::moveSelection(int delta) {
    if (picker_active_) {
        movePickerSelection(delta);
        return;
    }
    int count = static_cast<int>(buttons_.size());
    if (count <= 0) return;

    selected_index_ += delta;

    if (selected_index_ < 0) selected_index_ = count - 1;
    if (selected_index_ >= count) selected_index_ = 0;

    updateSelection();
}

void ScreenSettings::adjustValue(int delta) {
    if (picker_active_) {
        movePickerSelection(delta);
        return;
    }
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(item_keys_.size())) {
        return;
    }
    const std::string& key = item_keys_[selected_index_];
    if (key == "advanced" || key == "back") {
        return;
    }
    if (setting_change_cb_) {
        setting_change_cb_(key, delta, false, "");
    }
}

void ScreenSettings::selectCurrent() {
    if (picker_active_) {
        if (picker_index_ >= 0 && picker_index_ < static_cast<int>(picker_options_.size())) {
            const PickerOption& option = picker_options_[picker_index_];
            if (setting_change_cb_) {
                if (!option.id.empty()) {
                    setting_change_cb_(picker_key_, 0, true, option.id);
                } else {
                    setting_change_cb_(picker_key_, option.value, true, "");
                }
            }
        }
        hidePicker();
        return;
    }
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(item_keys_.size())) {
        return;
    }
    const std::string& key = item_keys_[selected_index_];

    if (key == "back") {
        goBack();
        return;
    }
    if (key == "advanced") {
        if (navigate_cb_) {
            navigate_cb_("advanced");
        }
        return;
    }
    if (key == "font_size" || key == "ui_scale" || key == "editor_font" ||
        key == "editor_margin" || key == "brightness") {
        showPicker(key);
        return;
    }
    if (setting_change_cb_) {
        setting_change_cb_(key, 1, false, "");
    }
}

void ScreenSettings::goBack() {
    if (in_advanced_) {
        in_advanced_ = false;
        // TODO: Return to main settings
    } else {
        hide();
    }
}

bool ScreenSettings::handleBack() {
    if (picker_active_) {
        hidePicker();
        return true;
    }
    return false;
}

void ScreenSettings::updateSelection() {
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (static_cast<int>(i) == selected_index_) {
            lv_obj_add_state(buttons_[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(buttons_[i], LV_STATE_CHECKED);
        }
    }
    lv_obj_invalidate(settings_list_);
}

void ScreenSettings::updateCurrentValue() {
    rebuildList();
}

void ScreenSettings::showPicker(const std::string& key) {
    if (!screen_) {
        return;
    }
    if (!picker_overlay_) {
        picker_overlay_ = lv_obj_create(screen_);
        lv_obj_clear_flag(picker_overlay_, LV_OBJ_FLAG_SCROLLABLE);

        picker_title_ = lv_label_create(picker_overlay_);
        lv_label_set_text(picker_title_, "");

        picker_roller_ = lv_roller_create(picker_overlay_);
        lv_obj_set_style_text_align(picker_roller_, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_add_flag(picker_overlay_, LV_OBJ_FLAG_HIDDEN);
    }

    picker_key_ = key;
    picker_options_.clear();
    picker_index_ = 0;

    Strings& strings = Strings::getInstance();
    if (picker_key_ == "font_size") {
        for (int size = Theme::FONT_SIZE_MIN; size <= Theme::FONT_SIZE_MAX; size += Theme::FONT_SIZE_STEP) {
            picker_options_.push_back({fontSizeLabel(size), size, ""});
            if (size == settings_.font_size) {
                picker_index_ = static_cast<int>(picker_options_.size() - 1);
            }
        }
        lv_label_set_text(picker_title_, strings.get("settings.font_size"));
    } else if (picker_key_ == "ui_scale") {
        static const int kScaleOptions[] = {50, 75, 100, 125, 150};
        for (int percent : kScaleOptions) {
            picker_options_.push_back({uiScaleLabel(percent), percent, ""});
            if (percent == settings_.ui_scale) {
                picker_index_ = static_cast<int>(picker_options_.size() - 1);
            }
        }
        lv_label_set_text(picker_title_, strings.get("settings.ui_scale"));
    } else if (picker_key_ == "editor_font") {
        size_t count = Theme::getEditorFontCount();
        for (size_t i = 0; i < count; ++i) {
            const char* id = Theme::getEditorFontIdByIndex(i);
            const char* label_key = Theme::getEditorFontLabelKey(id);
            picker_options_.push_back({strings.get(label_key), 0, id});
            if (settings_.editor_font_id == id) {
                picker_index_ = static_cast<int>(picker_options_.size() - 1);
            }
        }
        lv_label_set_text(picker_title_, strings.get("settings.editor_font"));
    } else if (picker_key_ == "editor_margin") {
        static const int kMarginOptions[] = {0, 1, 2};
        for (int value : kMarginOptions) {
            picker_options_.push_back({marginLabel(value), value, ""});
            if (settings_.editor_margin == value) {
                picker_index_ = static_cast<int>(picker_options_.size() - 1);
            }
        }
        lv_label_set_text(picker_title_, strings.get("settings.margin"));
    } else if (picker_key_ == "brightness") {
        int best_diff = 999;
        for (int value = 0; value <= 100; value += 10) {
            picker_options_.push_back({brightnessLabel(value), value, ""});
            int diff = settings_.brightness - value;
            if (diff < 0) diff = -diff;
            if (diff < best_diff) {
                best_diff = diff;
                picker_index_ = static_cast<int>(picker_options_.size() - 1);
            }
        }
        lv_label_set_text(picker_title_, strings.get("settings.brightness"));
    } else {
        return;
    }

    std::string options;
    for (size_t i = 0; i < picker_options_.size(); ++i) {
        options += picker_options_[i].label;
        if (i + 1 < picker_options_.size()) {
            options += "\n";
        }
    }

    picker_active_ = true;
    applyTheme();
    if (picker_roller_) {
        lv_roller_set_options(picker_roller_, options.c_str(), LV_ROLLER_MODE_NORMAL);
        lv_roller_set_selected(picker_roller_, picker_index_, LV_ANIM_OFF);
    }
    lv_obj_clear_flag(picker_overlay_, LV_OBJ_FLAG_HIDDEN);
    if (picker_overlay_) {
        lv_obj_move_foreground(picker_overlay_);
        lv_obj_update_layout(picker_overlay_);
    }
    if (picker_roller_) {
        lv_obj_update_layout(picker_roller_);
        lv_obj_invalidate(picker_roller_);
    }
    lv_obj_invalidate(picker_overlay_);
}

void ScreenSettings::hidePicker() {
    if (!picker_overlay_) {
        return;
    }
    lv_obj_add_flag(picker_overlay_, LV_OBJ_FLAG_HIDDEN);
    picker_active_ = false;
    picker_options_.clear();
}

void ScreenSettings::movePickerSelection(int delta) {
    if (!picker_active_ || picker_options_.empty()) {
        return;
    }
    int count = static_cast<int>(picker_options_.size());
    picker_index_ += delta;
    while (picker_index_ < 0) {
        picker_index_ += count;
    }
    picker_index_ = picker_index_ % count;
    lv_roller_set_selected(picker_roller_, picker_index_, LV_ANIM_OFF);
}

void ScreenSettings::applyTheme() {
    const Theme::Colors& colors = Theme::getColors();
    int overlay_w = 0;
    int overlay_h = 0;
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        lv_obj_set_style_text_font(screen_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
    }
    if (settings_list_) {
        lv_obj_set_size(settings_list_, Theme::fitWidth(350, 40), Theme::fitHeight(380, 140));
        lv_obj_align(settings_list_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(70));
        lv_obj_set_style_bg_color(settings_list_, colors.fg, 0);
        lv_obj_set_style_bg_opa(settings_list_, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(settings_list_, colors.border, 0);
        lv_obj_set_style_border_width(settings_list_, 1, 0);
    }
    if (title_label_) {
        lv_obj_set_style_text_color(title_label_, colors.text, 0);
        lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
        lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(30));
    }
    if (picker_overlay_) {
        lv_obj_set_style_bg_color(picker_overlay_, colors.fg, 0);
        lv_obj_set_style_bg_opa(picker_overlay_, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(picker_overlay_, colors.border, 0);
        lv_obj_set_style_border_width(picker_overlay_, 1, 0);
        lv_obj_set_style_text_font(picker_overlay_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        overlay_w = Theme::fitWidth(360, 40);
        overlay_h = Theme::fitHeight(260, 120);
        lv_obj_set_size(picker_overlay_, overlay_w, overlay_h);
        lv_obj_center(picker_overlay_);
    }
    if (picker_title_) {
        lv_obj_set_style_text_color(picker_title_, colors.text, 0);
        lv_obj_set_style_text_font(picker_title_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
        lv_obj_align(picker_title_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(14));
    }
    if (picker_roller_) {
        int base_overlay_w = overlay_w > 0 ? overlay_w : Theme::fitWidth(360, 40);
        int base_overlay_h = overlay_h > 0 ? overlay_h : Theme::fitHeight(260, 120);
        int roller_w = Theme::fitWidth(300, 80);
        int roller_h = Theme::fitHeight(180, 160);
        int max_w = base_overlay_w - Theme::scalePx(40);
        int max_h = base_overlay_h - Theme::scalePx(70);
        if (roller_w > max_w) roller_w = max_w;
        if (roller_h > max_h) roller_h = max_h;
        if (roller_w < 1) roller_w = 1;
        if (roller_h < 1) roller_h = 1;
        lv_obj_set_style_bg_color(picker_roller_, colors.bg, 0);
        lv_obj_set_style_bg_opa(picker_roller_, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(picker_roller_, colors.border, 0);
        lv_obj_set_style_border_width(picker_roller_, 1, 0);
        lv_obj_set_style_text_color(picker_roller_, colors.text, 0);
        lv_obj_set_style_text_font(picker_roller_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        lv_obj_set_size(picker_roller_, roller_w, roller_h);
        lv_obj_align(picker_roller_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(16));
        lv_roller_set_visible_row_count(picker_roller_, 5);
    }
}
