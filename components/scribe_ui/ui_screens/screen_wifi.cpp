#include "screen_wifi.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_WIFI";

ScreenWiFi::ScreenWiFi() : screen_(nullptr), wifi_enabled_(false), wifi_connected_(false) {
}

ScreenWiFi::~ScreenWiFi() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenWiFi::init() {
    ESP_LOGI(TAG, "Initializing WiFi settings screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenWiFi::createWidgets() {
    const Theme::Colors& colors = Theme::getColors();
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("settings.wifi"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(40));
    lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
    lv_obj_set_style_text_color(title_label_, colors.text, 0);

    // Status label
    status_label_ = lv_label_create(screen_);
    lv_label_set_text(status_label_, Strings::getInstance().get("settings.wifi_off"));
    lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(80));
    lv_obj_set_style_text_color(status_label_, colors.text, 0);
    lv_obj_set_style_text_font(status_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Connection detail
    ssid_label_ = lv_label_create(screen_);
    lv_label_set_text(ssid_label_, Strings::getInstance().get("wifi.status_disconnected"));
    lv_obj_align(ssid_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(104));
    lv_obj_set_style_text_color(ssid_label_, colors.text_secondary, 0);
    lv_obj_set_style_text_font(ssid_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);

    // WiFi switch
    wifi_switch_ = lv_switch_create(screen_);
    lv_obj_align(wifi_switch_, LV_ALIGN_TOP_MID, -Theme::scalePx(50), Theme::scalePx(120));
    lv_obj_add_event_cb(wifi_switch_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenWiFi* screen = (ScreenWiFi*)lv_obj_get_user_data(target);
        if (screen) {
            screen->toggleWiFi(target, e);
        }
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_set_user_data(wifi_switch_, this);

    on_label_ = lv_label_create(screen_);
    lv_label_set_text(on_label_, Strings::getInstance().get("common.on"));
    lv_obj_align_to(on_label_, wifi_switch_, LV_ALIGN_OUT_RIGHT_MID, Theme::scalePx(20), 0);
    lv_obj_set_style_text_font(on_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Network list
    list_ = lv_list_create(screen_);
    lv_obj_set_size(list_, Theme::fitWidth(360, 40), Theme::fitHeight(300, 220));
    lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(170));
    lv_obj_set_style_bg_color(list_, colors.fg, 0);
    lv_obj_set_style_bg_opa(list_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(list_, colors.border, 0);
    lv_obj_set_style_border_width(list_, 1, 0);
    lv_obj_set_style_text_font(list_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Scan button
    scan_btn_ = lv_button_create(screen_);
    lv_obj_set_size(scan_btn_, Theme::scalePx(120), Theme::scalePx(46));
    lv_obj_align(scan_btn_, LV_ALIGN_BOTTOM_LEFT, Theme::scalePx(30), -Theme::scalePx(30));
    lv_obj_set_style_text_font(scan_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
    lv_obj_add_event_cb(scan_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenWiFi* screen = (ScreenWiFi*)lv_obj_get_user_data(target);
        if (screen) {
            screen->requestScan();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(scan_btn_, this);

    scan_label_ = lv_label_create(scan_btn_);
    lv_label_set_text(scan_label_, Strings::getInstance().get("wifi.scan"));
    lv_obj_center(scan_label_);

    // Back button
    back_btn_ = lv_button_create(screen_);
    lv_obj_set_size(back_btn_, Theme::scalePx(120), Theme::scalePx(50));
    lv_obj_align(back_btn_, LV_ALIGN_BOTTOM_RIGHT, -Theme::scalePx(30), -Theme::scalePx(30));
    lv_obj_set_style_text_font(back_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    lv_obj_t* label = lv_label_create(back_btn_);
    lv_label_set_text(label, Strings::getInstance().get("common.back"));
    lv_obj_center(label);

    lv_obj_add_event_cb(back_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenWiFi* screen = (ScreenWiFi*)lv_obj_get_user_data(target);
        if (screen && screen->back_cb_) {
            screen->back_cb_();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(back_btn_, this);
}

void ScreenWiFi::show() {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        const Theme::Colors& colors = Theme::getColors();
        lv_obj_set_style_text_font(screen_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        if (title_label_) {
            lv_obj_set_style_text_color(title_label_, colors.text, 0);
            lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
            lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(40));
        }
        if (status_label_) {
            lv_obj_set_style_text_color(status_label_, colors.text, 0);
            lv_obj_set_style_text_font(status_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(80));
        }
        if (ssid_label_) {
            lv_obj_set_style_text_color(ssid_label_, colors.text_secondary, 0);
            lv_obj_set_style_text_font(ssid_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
            lv_obj_align(ssid_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(104));
        }
        if (wifi_switch_) {
            lv_obj_align(wifi_switch_, LV_ALIGN_TOP_MID, -Theme::scalePx(50), Theme::scalePx(120));
        }
        if (on_label_) {
            lv_obj_set_style_text_color(on_label_, colors.text, 0);
            lv_obj_set_style_text_font(on_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_align_to(on_label_, wifi_switch_, LV_ALIGN_OUT_RIGHT_MID, Theme::scalePx(20), 0);
        }
        if (list_) {
            lv_obj_set_size(list_, Theme::fitWidth(360, 40), Theme::fitHeight(300, 220));
            lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(170));
            lv_obj_set_style_bg_color(list_, colors.fg, 0);
            lv_obj_set_style_bg_opa(list_, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(list_, colors.border, 0);
            lv_obj_set_style_border_width(list_, 1, 0);
            lv_obj_set_style_text_font(list_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (scan_btn_) {
            lv_obj_set_size(scan_btn_, Theme::scalePx(120), Theme::scalePx(46));
            lv_obj_align(scan_btn_, LV_ALIGN_BOTTOM_LEFT, Theme::scalePx(30), -Theme::scalePx(30));
            lv_obj_set_style_text_font(scan_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (back_btn_) {
            lv_obj_set_size(back_btn_, Theme::scalePx(120), Theme::scalePx(50));
            lv_obj_align(back_btn_, LV_ALIGN_BOTTOM_RIGHT, -Theme::scalePx(30), -Theme::scalePx(30));
            lv_obj_set_style_text_font(back_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        lv_screen_load(screen_);
        updateStatus(wifi_enabled_, wifi_connected_);
        rebuildList();
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
    if (!enabled) {
        scanning_ = false;
        if (password_visible_) {
            hidePasswordPrompt();
        }
    }

    if (status_label_) {
        lv_label_set_text(status_label_, enabled
            ? Strings::getInstance().get("settings.wifi_on")
            : Strings::getInstance().get("settings.wifi_off"));
    }

    if (wifi_switch_) {
        if (enabled) {
            lv_obj_add_state(wifi_switch_, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(wifi_switch_, LV_STATE_CHECKED);
        }
    }

    if (ssid_label_) {
        if (!enabled) {
            lv_label_set_text(ssid_label_, Strings::getInstance().get("wifi.status_off"));
        } else if (connected && ssid && ssid[0] != '\0') {
            std::string text = Strings::getInstance().format("wifi.status_connected",
                {{"ssid", ssid}});
            lv_label_set_text(ssid_label_, text.c_str());
        } else {
            lv_label_set_text(ssid_label_, Strings::getInstance().get("wifi.status_disconnected"));
        }
    }

    rebuildList();
}

void ScreenWiFi::toggleWiFi(lv_obj_t* obj, lv_event_t* event) {
    (void)event;
    bool enabled = lv_obj_has_state(obj, LV_STATE_CHECKED);

    ESP_LOGI(TAG, "WiFi toggled: %s", enabled ? "on" : "off");

    if (toggle_cb_) {
        toggle_cb_(enabled);
        return;
    }

    wifi_enabled_ = enabled;
    updateStatus(enabled, false);
}

void ScreenWiFi::setNetworks(const std::vector<WiFiNetwork>& networks) {
    networks_ = networks;
    scanning_ = false;
    if (ssid_label_ && wifi_enabled_ && !wifi_connected_) {
        lv_label_set_text(ssid_label_, Strings::getInstance().get("wifi.status_disconnected"));
    }
    rebuildList();
}

void ScreenWiFi::setScanning(bool scanning) {
    scanning_ = scanning;
    if (!scanning_ && ssid_label_ && wifi_enabled_ && !wifi_connected_) {
        lv_label_set_text(ssid_label_, Strings::getInstance().get("wifi.status_disconnected"));
    }
    rebuildList();
}

bool ScreenWiFi::handleKeyEvent(const KeyEvent& event) {
    if (!event.pressed) {
        return false;
    }

    if (password_visible_) {
        if (event.key == KeyEvent::Key::ESC) {
            hidePasswordPrompt();
            return true;
        }
        if (event.key == KeyEvent::Key::ENTER) {
            if (!pending_ssid_.empty() && connect_cb_) {
                connect_cb_(pending_ssid_, password_);
            }
            if (ssid_label_ && !pending_ssid_.empty()) {
                std::string msg = Strings::getInstance().format("wifi.connecting",
                    {{"ssid", pending_ssid_}});
                lv_label_set_text(ssid_label_, msg.c_str());
            }
            hidePasswordPrompt();
            return true;
        }
        if (event.key == KeyEvent::Key::BACKSPACE) {
            backspacePassword();
            return true;
        }
        if (event.isPrintable() && !event.ctrl && !event.alt && !event.meta) {
            appendPasswordChar(event.char_code);
            return true;
        }
        return true;
    }

    if (event.key == KeyEvent::Key::UP || (event.key == KeyEvent::Key::TAB && event.shift)) {
        moveSelection(-1);
        return true;
    }
    if (event.key == KeyEvent::Key::DOWN || (event.key == KeyEvent::Key::TAB && !event.shift)) {
        moveSelection(1);
        return true;
    }
    if ((event.ctrl && event.key == KeyEvent::Key::R) || event.key == KeyEvent::Key::F5) {
        requestScan();
        return true;
    }
    if (event.key == KeyEvent::Key::ENTER) {
        if (!wifi_enabled_) {
            if (toggle_cb_) {
                toggle_cb_(true);
            }
            return true;
        }
        if (scanning_) {
            return true;
        }
        if (networks_.empty()) {
            requestScan();
            return true;
        }
        selectCurrent();
        return true;
    }

    return false;
}

void ScreenWiFi::rebuildList() {
    if (!list_) {
        return;
    }

    Strings& strings = Strings::getInstance();
    if (scan_label_) {
        lv_label_set_text(scan_label_, scanning_
            ? strings.get("wifi.scanning")
            : strings.get("wifi.scan"));
    }
    if (scan_btn_) {
        if (!wifi_enabled_ || scanning_) {
            lv_obj_add_state(scan_btn_, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(scan_btn_, LV_STATE_DISABLED);
        }
    }

    lv_obj_clean(list_);
    buttons_.clear();
    button_data_.clear();
    selected_index_ = 0;

    if (!wifi_enabled_) {
        lv_list_add_button(list_, LV_SYMBOL_CLOSE, strings.get("wifi.list_off"));
        return;
    }
    if (scanning_) {
        lv_list_add_button(list_, LV_SYMBOL_REFRESH, strings.get("wifi.scanning"));
        return;
    }
    if (networks_.empty()) {
        lv_list_add_button(list_, LV_SYMBOL_LIST, strings.get("wifi.no_networks"));
        return;
    }

    const Theme::Colors& colors = Theme::getColors();
    buttons_.reserve(networks_.size());
    button_data_.reserve(networks_.size());

    for (size_t i = 0; i < networks_.size(); ++i) {
        const auto& net = networks_[i];
        char buf[128];
        const char* lock = net.secure ? LV_SYMBOL_OK : LV_SYMBOL_WIFI;
        int rssi = net.rssi;
        snprintf(buf, sizeof(buf), "%s  %s (%ddBm)", lock, net.ssid.c_str(), rssi);
        lv_obj_t* btn = lv_list_add_button(list_, LV_SYMBOL_LIST, buf);
        lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
        buttons_.push_back(btn);

        button_data_.push_back({this, static_cast<int>(i)});
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* target = lv_event_get_target_obj(e);
            ButtonData* data = static_cast<ButtonData*>(lv_obj_get_user_data(target));
            if (!data || !data->screen) {
                return;
            }
            data->screen->selected_index_ = data->index;
            data->screen->updateSelection();
            data->screen->selectCurrent();
        }, LV_EVENT_CLICKED, nullptr);
        lv_obj_set_user_data(btn, &button_data_.back());
    }

    updateSelection();
}

void ScreenWiFi::updateSelection() {
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (static_cast<int>(i) == selected_index_) {
            lv_obj_add_state(buttons_[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(buttons_[i], LV_STATE_CHECKED);
        }
    }
    if (list_) {
        lv_obj_invalidate(list_);
    }
}

void ScreenWiFi::moveSelection(int delta) {
    int count = static_cast<int>(buttons_.size());
    if (count <= 0) {
        return;
    }
    selected_index_ += delta;
    if (selected_index_ < 0) selected_index_ = count - 1;
    if (selected_index_ >= count) selected_index_ = 0;
    updateSelection();
}

void ScreenWiFi::selectCurrent() {
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(networks_.size())) {
        return;
    }
    const auto& net = networks_[selected_index_];
    if (net.secure) {
        showPasswordPrompt(net.ssid);
        return;
    }
    if (connect_cb_) {
        connect_cb_(net.ssid, "");
    }
    if (ssid_label_) {
        std::string msg = Strings::getInstance().format("wifi.connecting",
            {{"ssid", net.ssid}});
        lv_label_set_text(ssid_label_, msg.c_str());
    }
}

void ScreenWiFi::requestScan() {
    if (!wifi_enabled_ || scanning_) {
        return;
    }
    scanning_ = true;
    if (ssid_label_) {
        lv_label_set_text(ssid_label_, Strings::getInstance().get("wifi.scanning"));
    }
    rebuildList();
    if (scan_cb_) {
        scan_cb_();
    }
}

void ScreenWiFi::showPasswordPrompt(const std::string& ssid) {
    if (!screen_) {
        return;
    }
    if (!password_overlay_) {
        password_overlay_ = lv_obj_create(screen_);
        lv_obj_clear_flag(password_overlay_, LV_OBJ_FLAG_SCROLLABLE);

        password_title_ = lv_label_create(password_overlay_);
        password_value_ = lv_label_create(password_overlay_);
        password_hint_ = lv_label_create(password_overlay_);
    }

    pending_ssid_ = ssid;
    password_.clear();
    password_visible_ = true;

    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(password_overlay_, colors.fg, 0);
    lv_obj_set_style_bg_opa(password_overlay_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(password_overlay_, colors.border, 0);
    lv_obj_set_style_border_width(password_overlay_, 1, 0);
    lv_obj_set_style_text_font(password_overlay_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
    lv_obj_set_size(password_overlay_, Theme::fitWidth(360, 40), Theme::fitHeight(200, 180));
    lv_obj_center(password_overlay_);

    if (password_title_) {
        std::string title = Strings::getInstance().format("wifi.password_title", {{"ssid", ssid}});
        lv_label_set_text(password_title_, title.c_str());
        lv_obj_align(password_title_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(14));
        lv_obj_set_style_text_font(password_title_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
        lv_obj_set_style_text_color(password_title_, colors.text, 0);
    }
    if (password_value_) {
        updatePasswordLabel();
        lv_obj_align(password_value_, LV_ALIGN_CENTER, 0, -Theme::scalePx(6));
        lv_obj_set_style_text_font(password_value_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        lv_obj_set_style_text_color(password_value_, colors.text, 0);
    }
    if (password_hint_) {
        lv_label_set_text(password_hint_, Strings::getInstance().get("wifi.password_hint"));
        lv_obj_align(password_hint_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(12));
        lv_obj_set_style_text_font(password_hint_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
        lv_obj_set_style_text_color(password_hint_, colors.text_secondary, 0);
    }

    lv_obj_clear_flag(password_overlay_, LV_OBJ_FLAG_HIDDEN);
}

void ScreenWiFi::hidePasswordPrompt() {
    if (!password_overlay_) {
        password_visible_ = false;
        return;
    }
    lv_obj_add_flag(password_overlay_, LV_OBJ_FLAG_HIDDEN);
    password_visible_ = false;
    pending_ssid_.clear();
    password_.clear();
}

void ScreenWiFi::appendPasswordChar(char c) {
    if (c == '\0') {
        return;
    }
    if (password_.size() >= 63) {
        return;
    }
    password_.push_back(c);
    updatePasswordLabel();
}

void ScreenWiFi::backspacePassword() {
    if (!password_.empty()) {
        password_.pop_back();
        updatePasswordLabel();
    }
}

void ScreenWiFi::updatePasswordLabel() {
    if (!password_value_) {
        return;
    }
    if (password_.empty()) {
        lv_label_set_text(password_value_, Strings::getInstance().get("wifi.password_empty"));
        return;
    }
    std::string masked(password_.size(), '*');
    lv_label_set_text(password_value_, masked.c_str());
}
