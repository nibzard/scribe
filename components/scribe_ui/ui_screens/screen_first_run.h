#pragma once

#include <lvgl.h>
#include <functional>

// First run tip screen - Shown on first boot
class ScreenFirstRun {
public:
    using DismissCallback = std::function<void()>;

    ScreenFirstRun();
    ~ScreenFirstRun();

    void init();
    void show();
    void hide();

    void setDismissCallback(DismissCallback cb) { dismiss_cb_ = cb; }

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* tagline_label_ = nullptr;
    lv_obj_t* content_ = nullptr;
    lv_obj_t* tip_label_ = nullptr;
    lv_obj_t* dismiss_label_ = nullptr;
    DismissCallback dismiss_cb_;

    void createWidgets();
};
