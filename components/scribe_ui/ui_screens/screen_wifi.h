#pragma once

#include <lvgl.h>
#include <string>
#include <functional>
#include "key_event.h"
#include "wifi_manager.h"

// WiFi settings screen - Enable/disable WiFi, show connection status
class ScreenWiFi {
public:
    using BackCallback = std::function<void()>;
    using ToggleCallback = std::function<void(bool enabled)>;
    using ScanCallback = std::function<void()>;
    using ConnectCallback = std::function<void(const std::string& ssid, const std::string& password)>;

    ScreenWiFi();
    ~ScreenWiFi();

    void init();
    void show();
    void hide();

    void setBackCallback(BackCallback cb) { back_cb_ = cb; }
    void setToggleCallback(ToggleCallback cb) { toggle_cb_ = cb; }
    void setScanCallback(ScanCallback cb) { scan_cb_ = cb; }
    void setConnectCallback(ConnectCallback cb) { connect_cb_ = cb; }

    void updateStatus(bool enabled, bool connected, const char* ssid = nullptr);
    void setNetworks(const std::vector<WiFiNetwork>& networks);
    void setScanning(bool scanning);
    bool handleKeyEvent(const KeyEvent& event);

private:
    struct ButtonData {
        ScreenWiFi* screen = nullptr;
        int index = 0;
    };

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* wifi_switch_ = nullptr;
    lv_obj_t* on_label_ = nullptr;
    lv_obj_t* back_btn_ = nullptr;
    lv_obj_t* ssid_label_ = nullptr;
    lv_obj_t* scan_btn_ = nullptr;
    lv_obj_t* scan_label_ = nullptr;
    lv_obj_t* list_ = nullptr;
    lv_obj_t* password_overlay_ = nullptr;
    lv_obj_t* password_title_ = nullptr;
    lv_obj_t* password_value_ = nullptr;
    lv_obj_t* password_hint_ = nullptr;

    BackCallback back_cb_;
    ToggleCallback toggle_cb_;
    ScanCallback scan_cb_;
    ConnectCallback connect_cb_;

    bool wifi_enabled_ = false;
    bool wifi_connected_ = false;
    bool scanning_ = false;
    bool password_visible_ = false;
    int selected_index_ = 0;
    std::string pending_ssid_;
    std::string password_;
    std::vector<WiFiNetwork> networks_;
    std::vector<lv_obj_t*> buttons_;
    std::vector<ButtonData> button_data_;

    void createWidgets();
    void toggleWiFi(lv_obj_t* obj, lv_event_t* event);
    void rebuildList();
    void updateSelection();
    void moveSelection(int delta);
    void selectCurrent();
    void requestScan();
    void showPasswordPrompt(const std::string& ssid);
    void hidePasswordPrompt();
    void appendPasswordChar(char c);
    void backspacePassword();
    void updatePasswordLabel();
};
