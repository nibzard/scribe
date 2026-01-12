#pragma once

#include <lvgl.h>
#include <string>
#include <functional>
#include <vector>

// Advanced settings screen - WiFi, Backup, AI, Diagnostics
class ScreenAdvanced {
public:
    using BackCallback = std::function<void()>;
    using NavigateCallback = std::function<void(const char* destination)>;

    ScreenAdvanced();
    ~ScreenAdvanced();

    void init();
    void show();
    void hide();
    void moveSelection(int delta);
    void selectCurrent();

    void setBackCallback(BackCallback cb) { back_cb_ = cb; }
    void setNavigateCallback(NavigateCallback cb) { navigate_cb_ = cb; }

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* settings_list_ = nullptr;
    int selected_index_ = 0;
    std::vector<lv_obj_t*> buttons_;
    std::vector<std::string> item_keys_;

    BackCallback back_cb_;
    NavigateCallback navigate_cb_;

    void createWidgets();
    void updateSelection();
};
