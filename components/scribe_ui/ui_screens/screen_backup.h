#pragma once

#include <lvgl.h>
#include <string>
#include <functional>

// Backup settings screen - Cloud backup setup (GitHub/Gist)
class ScreenBackup {
public:
    using BackCallback = std::function<void()>;

    ScreenBackup();
    ~ScreenBackup();

    void init();
    void show();
    void hide();

    void setBackCallback(BackCallback cb) { back_cb_ = cb; }

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* description_label_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* provider_list_ = nullptr;

    BackCallback back_cb_;

    void createWidgets();
    void updateStatus();
};
