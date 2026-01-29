#pragma once

#include <lvgl.h>
#include <functional>

// USB storage screen - shows SD card USB export state
class ScreenUsbStorage {
public:
    using BackCallback = std::function<void()>;

    ScreenUsbStorage();
    ~ScreenUsbStorage();

    void init();
    void show();
    void hide();

    void setBackCallback(BackCallback cb) { back_cb_ = cb; }
    void setConnected(bool connected);

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* content_ = nullptr;
    lv_obj_t* body_label_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* hint_label_ = nullptr;

    BackCallback back_cb_;
    bool connected_ = false;

    void createWidgets();
};
