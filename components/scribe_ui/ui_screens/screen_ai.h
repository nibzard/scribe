#pragma once

#include <lvgl.h>
#include <string>
#include <functional>

// AI settings screen - Configure OpenAI API
class ScreenAI {
public:
    using BackCallback = std::function<void()>;

    ScreenAI();
    ~ScreenAI();

    void init();
    void show();
    void hide();

    void setBackCallback(BackCallback cb) { back_cb_ = cb; }

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* description_label_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* api_key_input_ = nullptr;
    lv_obj_t* save_btn_ = nullptr;
    lv_obj_t* back_btn_ = nullptr;

    BackCallback back_cb_;

    void createWidgets();
    void updateStatus();

    // Save API key
    void saveAPIKey();
};
