#include "screen_settings.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_SETTINGS";

static const char* themeLabel(bool dark) {
    return dark ? "Dark" : "Light";
}

static const char* fontSizeLabel(int size) {
    switch (size) {
        case 0: return "Small";
        case 1: return "Medium";
        case 2: return "Large";
        default: return "Medium";
    }
}

static const char* autoSleepLabel(int value) {
    switch (value) {
        case 0: return "Off";
        case 1: return "5 minutes";
        case 2: return "15 minutes";
        case 3: return "30 minutes";
        default: return "15 minutes";
    }
}

static const char* keyboardLayoutLabel(int value) {
    switch (value) {
        case 0: return "US";
        case 1: return "UK";
        case 2: return "DE";
        case 3: return "FR";
        default: return "US";
    }
}

ScreenSettings::ScreenSettings() : screen_(nullptr), selected_index_(0), in_advanced_(false) {
}

ScreenSettings::~ScreenSettings() {
    if (screen_) {
        lv_obj_del(screen_);
    }
}

void ScreenSettings::init() {
    ESP_LOGI(TAG, "Initializing settings screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenSettings::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, "Settings");
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

    auto addItem = [&](const char* label, const char* value, const char* key, const char* symbol) {
        lv_obj_t* btn = lv_list_add_btn(settings_list_, symbol, label);
        buttons_.push_back(btn);
        item_keys_.push_back(key);

        lv_obj_t* value_label = nullptr;
        if (value) {
            value_label = lv_label_create(btn);
            lv_label_set_text(value_label, value);
            lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, -10, 0);
        }
        value_labels_.push_back(value_label);
    };

    addItem("Theme", themeLabel(settings_.dark_theme), "theme", LV_SYMBOL_IMAGE);
    addItem("Font size", fontSizeLabel(settings_.font_size), "font_size", LV_SYMBOL_FONT);
    addItem("Keyboard layout", keyboardLayoutLabel(settings_.keyboard_layout), "keyboard_layout", LV_SYMBOL_KEYBOARD);
    addItem("Auto-sleep", autoSleepLabel(settings_.auto_sleep), "auto_sleep", LV_SYMBOL_GPS);
    addItem("Advanced", nullptr, "advanced", LV_SYMBOL_SETTINGS);
    addItem("Back", nullptr, "back", LV_SYMBOL_LEFT);

    if (selected_index_ >= static_cast<int>(buttons_.size())) {
        selected_index_ = 0;
    }
    updateSelection();
}

void ScreenSettings::show() {
    if (screen_) {
        lv_scr_load(screen_);
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
