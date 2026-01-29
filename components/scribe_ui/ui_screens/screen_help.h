#pragma once

#include <lvgl.h>
#include <functional>
#include <vector>

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
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* content_ = nullptr;
    lv_obj_t* close_label_ = nullptr;
    std::vector<lv_obj_t*> heading_labels_;
    CloseCallback close_cb_;

    void createWidgets();
};
