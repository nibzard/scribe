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

static std::string fontSizeLabel(int size) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d px", size);
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
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_20, 0);

    // Settings list
    settings_list_ = lv_list_create(screen_);
    lv_obj_set_size(settings_list_, 350, 380);
    lv_obj_align(settings_list_, LV_ALIGN_TOP_MID, 0, 70);
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
            lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, -10, 0);
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
            lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, -90, 0);
        }

        lv_obj_t* swatch_row = lv_obj_create(theme_btn);
        lv_obj_set_size(swatch_row, 70, 14);
        lv_obj_clear_flag(swatch_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(swatch_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(swatch_row, 0, 0);
        lv_obj_align(swatch_row, LV_ALIGN_RIGHT_MID, -10, 0);

        const lv_color_t swatches[] = {colors.bg, colors.fg, colors.accent, colors.selection};
        for (size_t i = 0; i < sizeof(swatches) / sizeof(swatches[0]); ++i) {
            lv_obj_t* swatch = lv_obj_create(swatch_row);
            lv_obj_set_size(swatch, 12, 12);
            lv_obj_clear_flag(swatch, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(swatch, swatches[i], 0);
            lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(swatch, colors.border, 0);
            lv_obj_set_style_border_width(swatch, 1, 0);
            lv_obj_align(swatch, LV_ALIGN_LEFT_MID, static_cast<int>(i) * 16, 0);
        }
    }
    addItem(strings.get("settings.orientation"), orientationLabel(settings_.display_orientation),
            "display_orientation", LV_SYMBOL_LOOP);
    std::string font_label = fontSizeLabel(settings_.font_size);
    addItem(strings.get("settings.font_size"), font_label.c_str(), "font_size", LV_SYMBOL_EDIT);
    addItem(strings.get("settings.keyboard_layout"), keyboardLayoutLabel(settings_.keyboard_layout), "keyboard_layout", LV_SYMBOL_KEYBOARD);
    addItem(strings.get("settings.auto_sleep"), autoSleepLabel(settings_.auto_sleep), "auto_sleep", LV_SYMBOL_GPS);
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
    if (close_cb_) {
        close_cb_();
    }
}

void ScreenSettings::moveSelection(int delta) {
    int count = static_cast<int>(buttons_.size());
    if (count <= 0) return;

    selected_index_ += delta;

    if (selected_index_ < 0) selected_index_ = count - 1;
    if (selected_index_ >= count) selected_index_ = 0;

    updateSelection();
}

void ScreenSettings::adjustValue(int delta) {
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(item_keys_.size())) {
        return;
    }
    const std::string& key = item_keys_[selected_index_];
    if (key == "advanced" || key == "back") {
        return;
    }
    if (setting_change_cb_) {
        setting_change_cb_(key, delta);
    }
}

void ScreenSettings::selectCurrent() {
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
    if (setting_change_cb_) {
        setting_change_cb_(key, 1);
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

void ScreenSettings::applyTheme() {
    const Theme::Colors& colors = Theme::getColors();
    if (screen_) {
        Theme::applyScreenStyle(screen_);
    }
    if (settings_list_) {
        lv_obj_set_style_bg_color(settings_list_, colors.fg, 0);
        lv_obj_set_style_bg_opa(settings_list_, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(settings_list_, colors.border, 0);
        lv_obj_set_style_border_width(settings_list_, 1, 0);
    }
    if (title_label_) {
        lv_obj_set_style_text_color(title_label_, colors.text, 0);
    }
}
