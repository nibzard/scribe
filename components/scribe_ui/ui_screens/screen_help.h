#pragma once

#include <lvgl.h>
#include <functional>

// Help screen - Shows keyboard shortcuts and usage info
class ScreenHelp {
public:
    using CloseCallback = std::function<void()>;

    ScreenHelp();
    ~ScreenHelp();

    void init();
    void show();
    void hide();

    void setCloseCallback(CloseCallback cb) { close_cb_ = cb; }

private:
    lv_obj_t* screen_ = nullptr;
    CloseCallback close_cb_;

    void createWidgets();
};
