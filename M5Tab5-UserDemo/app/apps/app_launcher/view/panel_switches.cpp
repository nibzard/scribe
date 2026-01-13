/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <assets/assets.h>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>
#include <apps/utils/ui/toast.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag                           = "panel-switches";
static const std::string _toast_msg_charge_enable       = "Battery charging enabled";
static const std::string _toast_msg_charge_disable      = "Battery charging disabled";
static const std::string _toast_msg_charge_qc_enable    = "QC charging enabled";
static const std::string _toast_msg_charge_qc_disable   = "QC charging disabled";
static const std::string _toast_msg_ext_5v_enable       = " EXT 5V enabled";
static const std::string _toast_msg_ext_5v_disable      = "EXT 5V disabled";
static const std::string _toast_msg_usba_5v_enable      = "USB-A 5V enabled";
static const std::string _toast_msg_usba_5v_disable     = "USB-A 5V disabled";
static const std::string _toast_msg_ext_antenna_enable  = "Switched to external antenna";
static const std::string _toast_msg_ext_antenna_disable = "Switched to internal antenna";
static const std::string _toast_msg_usb_c_connected     = "USB-C connected";
static const std::string _toast_msg_usb_c_disconnected  = "USB-C disconnected";
static const std::string _toast_msg_usb_a_connected     = "USB-A connected";
static const std::string _toast_msg_usb_a_disconnected  = "USB-A disconnected";
static const std::string _toast_msg_hp_connected        = "Headphone connected";
static const std::string _toast_msg_hp_disconnected     = "Headphone disconnected";

static const ui::Window::KeyFrame_t _kf_ap_msp_close = {-309, -187, 60, 60, 0};
static const ui::Window::KeyFrame_t _kf_ap_msg_open  = {34, -70, 602, 332, 255};

class WifiApMsgWindow : public ui::Window {
public:
    WifiApMsgWindow()
    {
        config.kfClosed     = _kf_ap_msp_close;
        config.kfOpened     = _kf_ap_msg_open;
        config.bgColor      = 0x483333;
        config.clickBgClose = false;
        config.closeBtn     = true;
    }

    void onOpen() override
    {
        _window->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);

        _label_a = std::make_unique<Label>(_window->get());
        _label_a->align(LV_ALIGN_LEFT_MID, 32, -80);
        _label_a->setTextFont(&lv_font_montserrat_24);
        _label_a->setTextColor(lv_color_hex(0xFFFFFF));
        _label_a->setText("Connect to:");

        _label_b = std::make_unique<Label>(_window->get());
        _label_b->align(LV_ALIGN_LEFT_MID, 32, 0);
        _label_b->setTextFont(&lv_font_montserrat_24);
        _label_b->setTextColor(lv_color_hex(0xFFFFFF));
        _label_b->setText("And open url:");

        _panel_ssid = std::make_unique<Container>(_window->get());
        _panel_ssid->align(LV_ALIGN_CENTER, 87, -80);
        _panel_ssid->setSize(379, 51);
        _panel_ssid->setRadius(22);
        _panel_ssid->setBorderWidth(0);
        _panel_ssid->setBgColor(lv_color_hex(0x725151));

        _label_ssid = std::make_unique<Label>(_panel_ssid->get());
        _label_ssid->align(LV_ALIGN_CENTER, 0, 0);
        _label_ssid->setTextFont(&lv_font_montserrat_24);
        _label_ssid->setTextColor(lv_color_hex(0xFFFFFF));
        _label_ssid->setText("M5Tab5-UserDemo-WiFi");

        _panel_url = std::make_unique<Container>(_window->get());
        _panel_url->align(LV_ALIGN_CENTER, 105, 0);
        _panel_url->setSize(336, 51);
        _panel_url->setRadius(22);
        _panel_url->setBorderWidth(0);
        _panel_url->setBgColor(lv_color_hex(0x725151));

        _label_url = std::make_unique<Label>(_panel_url->get());
        _label_url->align(LV_ALIGN_CENTER, 0, 0);
        _label_url->setTextFont(&lv_font_montserrat_24);
        _label_url->setTextColor(lv_color_hex(0xFFFFFF));
        _label_url->setText("http://192.168.4.1");

        _panel_msg = std::make_unique<Container>(_window->get());
        _panel_msg->align(LV_ALIGN_CENTER, 0, 96);
        _panel_msg->setSize(547, 77);
        _panel_msg->setRadius(22);
        _panel_msg->setBorderWidth(0);
        _panel_msg->setBgColor(lv_color_hex(0x5A4141));
        _panel_msg->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

        _last_ext_antenna_enable = !GetHAL()->getExtAntennaEnable();
        update_msg();
    }

    void onUpdate() override
    {
        if (_state != Opened) {
            return;
        }

        if (GetHAL()->millis() - _time_count > 200) {
            update_msg();
        }
    }

    void onClose() override
    {
        _label_a.reset();
        _label_b.reset();
        _panel_ssid.reset();
        _label_ssid.reset();
        _panel_url.reset();
        _label_url.reset();
    }

private:
    uint32_t _time_count          = 0;
    bool _last_ext_antenna_enable = false;

    std::unique_ptr<Label> _label_a;
    std::unique_ptr<Label> _label_b;
    std::unique_ptr<Label> _label_ssid;
    std::unique_ptr<Label> _label_url;
    std::unique_ptr<Label> _label_msg_a;
    std::unique_ptr<Label> _label_msg_b;
    std::unique_ptr<Container> _panel_ssid;
    std::unique_ptr<Container> _panel_url;
    std::unique_ptr<Container> _panel_msg;

    void update_msg()
    {
        if (_last_ext_antenna_enable == GetHAL()->getExtAntennaEnable()) {
            return;
        }
        _last_ext_antenna_enable = GetHAL()->getExtAntennaEnable();

        if (GetHAL()->getExtAntennaEnable()) {
            _label_msg_a = std::make_unique<Label>(_panel_msg->get());
            _label_msg_a->align(LV_ALIGN_CENTER, 0, -13);
            _label_msg_a->setTextFont(&lv_font_montserrat_22);
            _label_msg_a->setTextColor(lv_color_hex(0xFFFFFF));
            _label_msg_a->setText("Using external antenna");

            _label_msg_b = std::make_unique<Label>(_panel_msg->get());
            _label_msg_b->align(LV_ALIGN_CENTER, 0, 13);
            _label_msg_b->setTextFont(&lv_font_montserrat_22);
            _label_msg_b->setTextColor(lv_color_hex(0xFFFFFF));
            _label_msg_b->setText("please make sure it's connected.");
        } else {
            _label_msg_a = std::make_unique<Label>(_panel_msg->get());
            _label_msg_a->align(LV_ALIGN_CENTER, 0, 0);
            _label_msg_a->setTextFont(&lv_font_montserrat_22);
            _label_msg_a->setTextColor(lv_color_hex(0xFFFFFF));
            _label_msg_a->setText("Using internal antenna.");

            _label_msg_b.reset();
        }
    }
};

void PanelSwitches::init()
{
    _img_charge_en_sw = std::make_unique<Image>(lv_screen_active());
    _img_charge_en_sw->align(LV_ALIGN_CENTER, 285, -73);

    _img_charge_qc_en_sw = std::make_unique<Image>(lv_screen_active());
    _img_charge_qc_en_sw->align(LV_ALIGN_CENTER, 285, -181);

    _img_ext_5v_en_sw = std::make_unique<Image>(lv_screen_active());
    _img_ext_5v_en_sw->align(LV_ALIGN_CENTER, -107, 64);

    _img_usba_5v_en_sw = std::make_unique<Image>(lv_screen_active());
    _img_usba_5v_en_sw->align(LV_ALIGN_CENTER, -202, -27);

    _img_ext_antenna_en_sw = std::make_unique<Image>(lv_screen_active());
    _img_ext_antenna_en_sw->align(LV_ALIGN_CENTER, -383, -191);

    _btn_charge_en_sw = std::make_unique<Container>(lv_screen_active());
    _btn_charge_en_sw->align(LV_ALIGN_CENTER, 285, -73);
    _btn_charge_en_sw->setSize(130, 75);
    _btn_charge_en_sw->setOpa(0);
    _btn_charge_en_sw->onClick().connect([&]() {
        audio::play_random_tone();
        GetHAL()->setChargeEnable(!GetHAL()->getChargeEnable());
        ui::pop_a_toast(GetHAL()->getChargeEnable() ? _toast_msg_charge_enable : _toast_msg_charge_disable,
                        GetHAL()->getChargeEnable() ? ui::toast_type::rose : ui::toast_type::gray);
        update_images();
    });

    _btn_charge_qc_en_sw = std::make_unique<Container>(lv_screen_active());
    _btn_charge_qc_en_sw->align(LV_ALIGN_CENTER, 285, -181);
    _btn_charge_qc_en_sw->setSize(130, 75);
    _btn_charge_qc_en_sw->setOpa(0);
    _btn_charge_qc_en_sw->onClick().connect([&]() {
        audio::play_random_tone();
        GetHAL()->setChargeQcEnable(!GetHAL()->getChargeQcEnable());
        ui::pop_a_toast(GetHAL()->getChargeQcEnable() ? _toast_msg_charge_qc_enable : _toast_msg_charge_qc_disable,
                        GetHAL()->getChargeQcEnable() ? ui::toast_type::orange : ui::toast_type::info);
        update_images();
    });

    _btn_ext_5v_en_sw = std::make_unique<Container>(lv_screen_active());
    _btn_ext_5v_en_sw->align(LV_ALIGN_CENTER, -107, 64);
    _btn_ext_5v_en_sw->setSize(130, 75);
    _btn_ext_5v_en_sw->setOpa(0);
    _btn_ext_5v_en_sw->onClick().connect([&]() {
        audio::play_random_tone();
        GetHAL()->setExt5vEnable(!GetHAL()->getExt5vEnable());
        ui::pop_a_toast(GetHAL()->getExt5vEnable() ? _toast_msg_ext_5v_enable : _toast_msg_ext_5v_disable,
                        GetHAL()->getExt5vEnable() ? ui::toast_type::error : ui::toast_type::success);
        update_images();
    });

    _btn_usba_5v_en_sw = std::make_unique<Container>(lv_screen_active());
    _btn_usba_5v_en_sw->align(LV_ALIGN_CENTER, -202, -27);
    _btn_usba_5v_en_sw->setSize(130, 75);
    _btn_usba_5v_en_sw->setOpa(0);
    _btn_usba_5v_en_sw->onClick().connect([&]() {
        audio::play_random_tone();
        GetHAL()->setUsb5vEnable(!GetHAL()->getUsb5vEnable());
        ui::pop_a_toast(GetHAL()->getUsb5vEnable() ? _toast_msg_usba_5v_enable : _toast_msg_usba_5v_disable,
                        GetHAL()->getUsb5vEnable() ? ui::toast_type::error : ui::toast_type::success);
        update_images();
    });

    _btn_ext_antenna_en_sw = std::make_unique<Container>(lv_screen_active());
    _btn_ext_antenna_en_sw->align(LV_ALIGN_CENTER, -383, -191);
    _btn_ext_antenna_en_sw->setSize(130, 75);
    _btn_ext_antenna_en_sw->setOpa(0);
    _btn_ext_antenna_en_sw->onClick().connect([&]() {
        audio::play_random_tone();
        GetHAL()->setExtAntennaEnable(!GetHAL()->getExtAntennaEnable());
        ui::pop_a_toast(
            GetHAL()->getExtAntennaEnable() ? _toast_msg_ext_antenna_enable : _toast_msg_ext_antenna_disable,
            ui::toast_type::info);
        update_images();

        if (!_window) {
            _window = std::make_unique<WifiApMsgWindow>();
            _window->init(lv_screen_active());
            _window->open();
        }
    });

    _btn_ap_msg = std::make_unique<Container>(lv_screen_active());
    _btn_ap_msg->align(LV_ALIGN_CENTER, -309, -187);
    _btn_ap_msg->setSize(60, 60);
    _btn_ap_msg->setOpa(0);
    _btn_ap_msg->onClick().connect([&]() {
        audio::play_random_tone();

        if (!_window) {
            _window = std::make_unique<WifiApMsgWindow>();
            _window->init(lv_screen_active());
            _window->open();
        }
    });

    update_images();

    _img_usb_c_detect = std::make_unique<Image>(lv_screen_active());
    _img_usb_c_detect->align(LV_ALIGN_CENTER, -627, 0);
    _img_usb_c_detect->setSrc(&arrow_state_on);

    _img_usb_a_detect = std::make_unique<Image>(lv_screen_active());
    _img_usb_a_detect->align(LV_ALIGN_CENTER, -627, -175);
    _img_usb_a_detect->setSrc(&arrow_state_on);

    _img_hp_detect = std::make_unique<Image>(lv_screen_active());
    _img_hp_detect->align(LV_ALIGN_CENTER, -627, 151);
    _img_hp_detect->setSrc(&arrow_state_on);

    _last_usb_c_detect = GetHAL()->usbCDetect();
    _last_usb_a_detect = GetHAL()->usbADetect();
    _last_hp_detect    = GetHAL()->headPhoneDetect();

    update_detect_images();
}

void PanelSwitches::update(bool isStacked)
{
    if (_window) {
        _window->update();
        if (_window->getState() == ui::Window::State_t::Closed) {
            _window.reset();
        }
    }

    if (GetHAL()->millis() - _time_count > 200) {
        update_images();
        update_detect_images();
        _time_count = GetHAL()->millis();
    }
}

void PanelSwitches::update_images()
{
    _img_charge_en_sw->setSrc(GetHAL()->getChargeEnable() ? &sw_chg_on : &sw_chg_off);
    _img_charge_qc_en_sw->setSrc(GetHAL()->getChargeQcEnable() ? &sw_qc_on : &sw_qc_off);
    _img_ext_5v_en_sw->setSrc(GetHAL()->getExt5vEnable() ? &sw_on : &sw_off);
    _img_usba_5v_en_sw->setSrc(GetHAL()->getUsb5vEnable() ? &sw_on : &sw_off);
    _img_ext_antenna_en_sw->setSrc(GetHAL()->getExtAntennaEnable() ? &sw_rf_h : &sw_rf_l);
}

void PanelSwitches::update_detect_images()
{
    _img_usb_c_detect->setOpa(GetHAL()->usbCDetect() ? 255 : 0);
    _img_usb_a_detect->setOpa(GetHAL()->usbADetect() ? 255 : 0);
    _img_hp_detect->setOpa(GetHAL()->headPhoneDetect() ? 255 : 0);

    // Handle state change
    // if (_last_usb_c_detect != GetHAL()->usbCDetect()) {
    //     _last_usb_c_detect = GetHAL()->usbCDetect();
    //     ui::pop_a_toast(_last_usb_c_detect ? _toast_msg_usb_c_connected : _toast_msg_usb_c_disconnected,
    //                     _last_usb_c_detect ? ui::toast_type::orange : ui::toast_type::gray);
    //     play_sfx(_last_usb_c_detect);
    // }
    if (_last_usb_a_detect != GetHAL()->usbADetect()) {
        _last_usb_a_detect = GetHAL()->usbADetect();
        ui::pop_a_toast(_last_usb_a_detect ? _toast_msg_usb_a_connected : _toast_msg_usb_a_disconnected,
                        _last_usb_a_detect ? ui::toast_type::orange : ui::toast_type::gray);
        play_sfx(_last_usb_a_detect);
    }
    if (_last_hp_detect != GetHAL()->headPhoneDetect()) {
        _last_hp_detect = GetHAL()->headPhoneDetect();
        ui::pop_a_toast(_last_hp_detect ? _toast_msg_hp_connected : _toast_msg_hp_disconnected,
                        _last_hp_detect ? ui::toast_type::orange : ui::toast_type::gray);
        play_sfx(_last_hp_detect);
    }
}

void PanelSwitches::play_sfx(bool isPlugedIn)
{
    if (isPlugedIn) {
        audio::play_melody({60 + 24, 64 + 24});
    } else {
        audio::play_melody({64 + 24, 60 + 24});
    }
}
