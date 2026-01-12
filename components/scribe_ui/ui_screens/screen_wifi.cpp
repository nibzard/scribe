#include "screen_wifi.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_WIFI";

ScreenWiFi::ScreenWiFi() : screen_(nullptr), wifi_enabled_(false), wifi_connected_(false) {
}

ScreenWiFi::~ScreenWiFi() {
    if (screen_) {
        lv_obj_del(screen_);
    }
}

void ScreenWiFi::init() {
    ESP_LOGI(TAG, "Initializing WiFi settings screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenWiFi::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, "Wi\u2011Fi");
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_20, 0);

    // Status label
    status_label_ = lv_label_create(screen_);
    lv_label_set_text(status_label_, "Off");
    lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, 80);

    // WiFi switch
    wifi_switch_ = lv_switch_create(screen_);
    lv_obj_align(wifi_switch_, LV_ALIGN_TOP_MID, -50, 120);
    lv_obj_add_event_cb(wifi_switch_, [](lv_obj_t* obj, lv_event_t* event) {
        ScreenWiFi* screen = (ScreenWiFi*)lv_obj_get_user_data(obj);
        if (screen) {
            screen->toggleWiFi(obj, event);
        }
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_set_user_data(wifi_switch_, this);

    lv_obj_t* label = lv_label_create(screen_);
    lv_label_set_text(label, "On");
    lv_obj_align_to(label, wifi_switch_, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    // Back button
    lv_obj_t* btn = lv_btn_create(screen_);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -30);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Back");
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, [](lv_obj_t* obj, lv_event_t* event) {
        ScreenWiFi* screen = (ScreenWiFi*)lv_obj_get_user_data(obj);
        if (screen && screen->back_cb_) {
            screen->back_cb_();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(btn, this);
}

void ScreenWiFi::show() {
    if (screen_) {
        lv_scr_load(screen_);
        updateStatus(wifi_enabled_, wifi_connected_);
    }
}

void ScreenWiFi::hide() {
    if (back_cb_) {
        back_cb_();
    }
}

void ScreenWiFi::updateStatus(bool enabled, bool connected, const char* ssid) {
    wifi_enabled_ = enabled;
    wifi_connected_ = connected;

    if (status_label_) {
        lv_label_set_text(status_label_, enabled ? "On" : "Off");
    }

    if (wifi_switch_) {
        if (enabled) {
            lv_obj_add_state(wifi_switch_, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(wifi_switch_, LV_STATE_CHECKED);
        }
    }

    (void)ssid;
}

void ScreenWiFi::toggleWiFi(lv_obj_t* obj, lv_event_t* event) {
    bool enabled = lv_obj_has_state(obj, LV_STATE_CHECKED);

    ESP_LOGI(TAG, "WiFi toggled: %s", enabled ? "on" : "off");

    // TODO: Enable/disable WiFi through WiFiManager
    wifi_enabled_ = enabled;
    updateStatus(enabled, false);

    if (enabled) {
        // TODO: Start WiFi scan and connection
        ESP_LOGI(TAG, "Starting WiFi...");
    } else {
        // TODO: Disconnect and turn off WiFi
        ESP_LOGI(TAG, "Stopping WiFi...");
    }
}
