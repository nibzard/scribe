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
    DismissCallback dismiss_cb_;

    void createWidgets();
};
