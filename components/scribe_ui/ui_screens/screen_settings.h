#pragma once

#include <lvgl.h>
#include <string>
#include <functional>
#include <vector>
#include "settings_store.h"

// Settings screen - Theme, font size, keyboard, auto-sleep
class ScreenSettings {
public:
    using CloseCallback = std::function<void()>;
    using SettingChangeCallback = std::function<void(const std::string& setting, int value, bool absolute,
                                                     const std::string& value_str)>;
    using NavigateCallback = std::function<void(const char* destination)>;

    ScreenSettings();
    ~ScreenSettings();

    void init();
    void show();
    void hide();

    void setCloseCallback(CloseCallback cb) { close_cb_ = cb; }
    void setSettingChangeCallback(SettingChangeCallback cb) { setting_change_cb_ = cb; }
    void setNavigateCallback(NavigateCallback cb) { navigate_cb_ = cb; }

    void setSettings(const AppSettings& settings);
    const AppSettings& getSettings() const { return settings_; }

    // Navigation
    void moveSelection(int delta);
    void adjustValue(int delta);
    void selectCurrent();
    void goBack();
    bool handleBack();

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* settings_list_ = nullptr;
    lv_obj_t* picker_overlay_ = nullptr;
    lv_obj_t* picker_title_ = nullptr;
    lv_obj_t* picker_roller_ = nullptr;

    int selected_index_ = 0;
    bool in_advanced_ = false;
    bool picker_active_ = false;
    int picker_index_ = 0;
    std::string picker_key_;
    struct PickerOption {
        std::string label;
        int value = 0;
        std::string id;
    };
    std::vector<PickerOption> picker_options_;
    std::vector<lv_obj_t*> buttons_;
    std::vector<lv_obj_t*> value_labels_;
    std::vector<std::string> item_keys_;
    AppSettings settings_;

    CloseCallback close_cb_;
    SettingChangeCallback setting_change_cb_;
    NavigateCallback navigate_cb_;

    void createWidgets();
    void rebuildList();
    void updateSelection();
    void updateCurrentValue();
    void applyTheme();
    void showPicker(const std::string& key);
    void hidePicker();
    void movePickerSelection(int delta);
};
