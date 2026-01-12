#pragma once

#include <lvgl.h>
#include <string>
#include <functional>

// WiFi settings screen - Enable/disable WiFi, show connection status
class ScreenWiFi {
public:
    using BackCallback = std::function<void()>;
    using ToggleCallback = std::function<void(bool enabled)>;

    ScreenWiFi();
    ~ScreenWiFi();

    void init();
    void show();
    void hide();

    void setBackCallback(BackCallback cb) { back_cb_ = cb; }
    void setToggleCallback(ToggleCallback cb) { toggle_cb_ = cb; }

    void updateStatus(bool enabled, bool connected, const char* ssid = nullptr);

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* wifi_switch_ = nullptr;
    lv_obj_t* ssid_label_ = nullptr;
    lv_obj_t* scan_btn_ = nullptr;

    BackCallback back_cb_;
    ToggleCallback toggle_cb_;

    bool wifi_enabled_ = false;
    bool wifi_connected_ = false;

    void createWidgets();
    void toggleWiFi(lv_obj_t* obj, lv_event_t* event);
};
