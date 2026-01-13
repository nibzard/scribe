/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <cstdint>
#include <lvgl.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-power";

static const ui::Window::KeyFrame_t _kf_power_off_close          = {562, -319, 146, 70, 0};
static const ui::Window::KeyFrame_t _kf_power_off_open           = {123, -200, 703, 291, 255};
static const ui::Window::KeyFrame_t _kf_sleep_touch_wakeup_close = {562, -245, 146, 70, 0};
static const ui::Window::KeyFrame_t _kf_sleep_touch_wakeup_open  = {123, -200, 703, 291, 255};
static const ui::Window::KeyFrame_t _kf_sleep_shake_wakeup_close = {562, -172, 146, 70, 0};
static const ui::Window::KeyFrame_t _kf_sleep_shake_wakeup_open  = {123, -163, 703, 291, 255};
static const ui::Window::KeyFrame_t _kf_sleep_rtc_wakeup_close   = {562, -98, 146, 70, 0};
static const ui::Window::KeyFrame_t _kf_sleep_rtc_wakeup_open    = {123, -116, 703, 291, 255};

class PowerWindowBase : public ui::Window {
public:
    void onInit() override
    {
        _label_header = std::make_unique<Label>(_window->get());
        _label_msg    = std::make_unique<Label>(_window->get());
        _btn_cancel   = std::make_unique<Button>(_window->get());
        _btn_confirm  = std::make_unique<Button>(_window->get());
    }

    static void apply_label_header_style(Label* label, int16_t x, int16_t y, const std::string& text)
    {
        label->align(LV_ALIGN_LEFT_MID, x, y);
        label->setTextColor(lv_color_hex(0xFFFFFF));
        label->setTextFont(&lv_font_montserrat_36);
        label->setText(text);
    }

    static void apply_label_msg_style(Label* label, int16_t x, int16_t y, const std::string& text)
    {
        label->align(LV_ALIGN_LEFT_MID, x, y);
        label->setTextColor(lv_color_hex(0xFFFFFF));
        label->setTextFont(&lv_font_montserrat_24);
        label->setText(text);
    }

    static void apply_button_style(Button* btn, int16_t x, int16_t y, uint32_t color, const std::string& label)
    {
        btn->align(LV_ALIGN_CENTER, x, y);
        btn->setSize(183, 58);
        btn->setRadius(18);
        btn->setBgColor(lv_color_hex(color));
        btn->setShadowWidth(0);
        btn->label().setText(label);
        btn->label().setTextColor(lv_color_hex(0xFFFFFF));
        btn->label().setTextFont(&lv_font_montserrat_24);
    }

protected:
    std::unique_ptr<Label> _label_header;
    std::unique_ptr<Label> _label_msg;
    std::unique_ptr<Button> _btn_cancel;
    std::unique_ptr<Button> _btn_confirm;
};

class PowerOffWindow : public PowerWindowBase {
public:
    PowerOffWindow()
    {
        config.kfClosed    = _kf_power_off_close;
        config.kfOpened    = _kf_power_off_open;
        config.bgColor     = 0xB14948;
        config.borderColor = 0x783A39;
    }

    void onOpen() override
    {
        apply_label_header_style(_label_header.get(), 50, -92, "Your device will shut down.");

        apply_label_msg_style(_label_msg.get(), 50, -21,
                              fmt::format("Shut down automatically in {} seconds.", _countdown));

        _btn_cancel->onClick().connect([&]() { close(); });
        apply_button_style(_btn_cancel.get(), -24, 78, 0x783A39, "Cancel");

        _btn_confirm->onClick().connect([&]() { GetHAL()->powerOff(); });
        apply_button_style(_btn_confirm.get(), 207, 78, 0xE65A57, "Shut Down");

        _time_count = GetHAL()->millis();
    }

    void onUpdate() override
    {
        if (GetHAL()->millis() - _time_count > 1000) {
            _countdown--;

            if (_countdown < 0) {
                GetHAL()->powerOff();
            }

            _label_msg->setText(fmt::format("Shut down automatically in {} seconds.", _countdown));
            _time_count = GetHAL()->millis();
        }
    }

    void onClose() override
    {
        audio::play_next_tone_progression();
    }

private:
    int _countdown       = 15;
    uint32_t _time_count = 0;
};

class SleepTouchWakeupWindow : public PowerWindowBase {
public:
    SleepTouchWakeupWindow()
    {
        config.kfClosed    = _kf_sleep_touch_wakeup_close;
        config.kfOpened    = _kf_sleep_touch_wakeup_open;
        config.bgColor     = 0x6B6D48;
        config.borderColor = 0x54553D;
    }

    void onOpen() override
    {
        apply_label_header_style(_label_header.get(), 50, -92, "Your device will enter sleepmode.");

        apply_label_msg_style(_label_msg.get(), 50, -21, "Touch the screen to wake it up.");

        _btn_cancel->onClick().connect([&]() { close(); });
        apply_button_style(_btn_cancel.get(), -24, 78, 0x54553D, "Cancel");

        _btn_confirm->onClick().connect([&]() {
            GetHAL()->sleepAndTouchWakeup();
            close();
        });
        apply_button_style(_btn_confirm.get(), 207, 78, 0x979A60, "Sleep");
    }

    void onClose() override
    {
        audio::play_next_tone_progression();
    }
};

class SleepShakeWakeupWindow : public PowerWindowBase {
public:
    SleepShakeWakeupWindow()
    {
        config.kfClosed    = _kf_sleep_shake_wakeup_close;
        config.kfOpened    = _kf_sleep_shake_wakeup_open;
        config.bgColor     = 0xB18F6C;
        config.borderColor = 0x896B4D;
    }

    void onOpen() override
    {
        apply_label_header_style(_label_header.get(), 50, -92, "Your device will shut down.");

        apply_label_msg_style(_label_msg.get(), 50, -21,
                              "Please place it still before confirming.\nShake the device to wake it up.");

        _btn_cancel->onClick().connect([&]() { close(); });
        apply_button_style(_btn_cancel.get(), -24, 78, 0x896B4D, "Cancel");

        _btn_confirm->onClick().connect([&]() { GetHAL()->sleepAndShakeWakeup(); });
        apply_button_style(_btn_confirm.get(), 207, 78, 0xD5AA7E, "Sleep");
    }

    void onClose() override
    {
        audio::play_next_tone_progression();
    }
};

class SleepRtcWakeupWindow : public PowerWindowBase {
public:
    SleepRtcWakeupWindow()
    {
        config.kfClosed    = _kf_sleep_rtc_wakeup_close;
        config.kfOpened    = _kf_sleep_rtc_wakeup_open;
        config.bgColor     = 0x637991;
        config.borderColor = 0x435A73;
    }

    void onOpen() override
    {
        apply_label_header_style(_label_header.get(), 50, -92, "Your device will shut down.");

        apply_label_msg_style(_label_msg.get(), 50, -21, "It will automatically wake up in 10 seconds.");

        _btn_cancel->onClick().connect([&]() { close(); });
        apply_button_style(_btn_cancel.get(), -24, 78, 0x435A73, "Cancel");

        _btn_confirm->onClick().connect([&]() { GetHAL()->sleepAndRtcWakeup(); });
        apply_button_style(_btn_confirm.get(), 207, 78, 0x709DCE, "Sleep");
    }
};

void PanelPower::init()
{
    _btn_power_off = std::make_unique<Container>(lv_screen_active());
    _btn_power_off->align(LV_ALIGN_CENTER, 562, -319);
    _btn_power_off->setSize(146, 70);
    _btn_power_off->setOpa(0);
    _btn_power_off->onClick().connect([&]() {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<PowerOffWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });

    _btn_sleep_touch_wakeup = std::make_unique<Container>(lv_screen_active());
    _btn_sleep_touch_wakeup->align(LV_ALIGN_CENTER, 562, -245);
    _btn_sleep_touch_wakeup->setSize(146, 70);
    _btn_sleep_touch_wakeup->setOpa(0);
    _btn_sleep_touch_wakeup->onClick().connect([&]() {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<SleepTouchWakeupWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });

    _btn_sleep_shake_wakeup = std::make_unique<Container>(lv_screen_active());
    _btn_sleep_shake_wakeup->align(LV_ALIGN_CENTER, 562, -172);
    _btn_sleep_shake_wakeup->setSize(146, 70);
    _btn_sleep_shake_wakeup->setOpa(0);
    _btn_sleep_shake_wakeup->onClick().connect([&]() {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<SleepShakeWakeupWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });

    _btn_sleep_rtc_wakeup = std::make_unique<Container>(lv_screen_active());
    _btn_sleep_rtc_wakeup->align(LV_ALIGN_CENTER, 562, -98);
    _btn_sleep_rtc_wakeup->setSize(146, 70);
    _btn_sleep_rtc_wakeup->setOpa(0);
    _btn_sleep_rtc_wakeup->onClick().connect([&]() {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<SleepRtcWakeupWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });
}

void PanelPower::update(bool isStacked)
{
    if (_window) {
        _window->update();
        if (_window->getState() == ui::Window::State_t::Closed) {
            _window.reset();
        }
    }
}
