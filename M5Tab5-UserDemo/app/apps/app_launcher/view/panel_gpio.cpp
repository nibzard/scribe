/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <functional>
#include <lvgl.h>
#include <hal/hal.h>
#include <memory>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>
#include <apps/utils/ui/window.h>
#include <apps/utils/ui/toast.h>
#include <array>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-gpio";

static const ui::Window::KeyFrame_t _kf_ext_port_close = {413, -115, 75, 75, 0};
static const ui::Window::KeyFrame_t _kf_ext_port_open  = {-438, 68, 273, 380, 255};
static const ui::Window::KeyFrame_t _kf_mbus_open      = {183, -15, 520, 547, 255};

class GpioOutputTestPanel {
public:
    std::function<void(uint8_t ioPin, bool level)> onToggle;

    void init(lv_obj_t* parent, uint8_t ioPin)
    {
        _io_pin = ioPin;

        _panel = std::make_unique<Container>(parent);
        _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _panel->setBgColor(lv_color_hex(0x383838));
        _panel->setPadding(0, 0, 0, 0);
        _panel->setSize(230, 62);
        _panel->setBorderWidth(0);
        _panel->setRadius(18);

        _label_io_num = std::make_unique<Label>(_panel->get());
        _label_io_num->align(LV_ALIGN_CENTER, -67, 0);
        _label_io_num->setTextFont(&lv_font_montserrat_28);
        _label_io_num->setText(fmt::format("G{}", _io_pin));

        _btn_io_toggle = std::make_unique<Button>(_panel->get());
        _btn_io_toggle->align(LV_ALIGN_CENTER, 47, 0);
        _btn_io_toggle->setSize(114, 42);
        _btn_io_toggle->setRadius(16);
        _btn_io_toggle->label().setTextFont(&lv_font_montserrat_22);
        _btn_io_toggle->onClick().connect([&]() {
            _is_on = !_is_on;
            update_btn_style();
            if (onToggle) {
                onToggle(_io_pin, _is_on);
            }
        });

        update_btn_style();
    }

    std::unique_ptr<Container>& get()
    {
        return _panel;
    }

    uint8_t getIoPin()
    {
        return _io_pin;
    }

protected:
    std::unique_ptr<Container> _panel;
    std::unique_ptr<Label> _label_io_num;
    std::unique_ptr<Button> _btn_io_toggle;
    bool _is_on     = false;
    uint8_t _io_pin = 0;

    void update_btn_style()
    {
        if (_is_on) {
            _btn_io_toggle->setBgColor(lv_color_hex(0xDD380D));
            _btn_io_toggle->label().setText("HIGH");
        } else {
            _btn_io_toggle->setBgColor(lv_color_hex(0x58B358));
            _btn_io_toggle->label().setText("LOW");
        }
    }
};

class MbusWindow : public ui::Window {
public:
    MbusWindow()
    {
        config.title        = "MBUS";
        config.kfClosed     = _kf_ext_port_close;
        config.kfOpened     = _kf_mbus_open;
        config.clickBgClose = false;
    }

    void onOpen() override
    {
        _window->setPadding(0, 24, 0, 0);

        _label_msg = std::make_unique<Label>(_window->get());
        _label_msg->align(LV_ALIGN_CENTER, 0, 0);
        _label_msg->setTextFont(&lv_font_montserrat_24);
        _label_msg->setTextColor(lv_color_hex(0xECEBEB));
        _label_msg->setText("Loading ...");
    }

    void onUpdate() override
    {
        if (_state == Opened) {
            if (_io_panels.empty()) {
                _label_msg.reset();
                for (int i = 0; i < _io_pins.size(); i++) {
                    GetHAL()->gpioReset(_io_pins[i]);
                    GetHAL()->gpioInitOutput(_io_pins[i]);
                    GetHAL()->gpioSetLevel(_io_pins[i], false);

                    _io_panels.push_back(std::make_unique<GpioOutputTestPanel>());
                    _io_panels.back()->init(_window->get(), _io_pins[i]);
                    _io_panels.back()->get()->align(LV_ALIGN_TOP_MID, i > 8 ? -124 : 124, 55 + 81 * (i % 9));
                    _io_panels.back()->onToggle = [&](uint8_t ioPin, bool isOn) {
                        audio::play_tone_from_midi(isOn ? (63 + 24) : (60 + 24));
                        GetHAL()->gpioSetLevel(ioPin, isOn);
                        mclog::tagInfo(_tag, "set G{}: {}", ioPin, isOn ? "high" : "low");
                    };
                }
            }
        }
    }

    void onClose() override
    {
        _io_panels.clear();
        for (const auto& io : _io_pins) {
            GetHAL()->gpioReset(io);
        }
    }

private:
    std::vector<std::unique_ptr<GpioOutputTestPanel>> _io_panels;
    std::unique_ptr<Label> _label_msg;
    const std::array<uint8_t, 18> _io_pins = {18, 19, 5, 38, 7, 3, 2, 47, 16, 17, 45, 52, 37, 6, 4, 48, 35, 51};
};

class ExtPortWindow : public ui::Window {
public:
    ExtPortWindow()
    {
        config.title    = "Ext.Port1";
        config.kfClosed = _kf_ext_port_close;
        config.kfOpened = _kf_ext_port_open;
    }

    void onOpen() override
    {
        // Ext port window
        _window->setPadding(0, 24, 0, 0);

        _label_msg = std::make_unique<Label>(_window->get());
        _label_msg->align(LV_ALIGN_CENTER, 0, 0);
        _label_msg->setTextFont(&lv_font_montserrat_24);
        _label_msg->setTextColor(lv_color_hex(0xECEBEB));
        _label_msg->setText("Loading ...");

        // Create mbus window
        _mbus_window = std::make_unique<MbusWindow>();
        _mbus_window->init(lv_screen_active());
        _mbus_window->open();
    }

    void onUpdate() override
    {
        if (_mbus_window) {
            _mbus_window->update();
        }

        if (_state == Opened) {
            if (_io_panels.empty()) {
                _label_msg.reset();
                for (int i = 0; i < _io_pins.size(); i++) {
                    GetHAL()->gpioReset(_io_pins[i]);
                    GetHAL()->gpioInitOutput(_io_pins[i]);
                    GetHAL()->gpioSetLevel(_io_pins[i], false);

                    _io_panels.push_back(std::make_unique<GpioOutputTestPanel>());
                    _io_panels.back()->init(_window->get(), _io_pins[i]);
                    _io_panels.back()->get()->align(LV_ALIGN_TOP_MID, 0, 55 + 81 * i);
                    _io_panels.back()->onToggle = [&](uint8_t ioPin, bool isOn) {
                        audio::play_tone_from_midi(isOn ? (63 + 24) : (60 + 24));
                        GetHAL()->gpioSetLevel(ioPin, isOn);
                        mclog::tagInfo(_tag, "set G{}: {}", ioPin, isOn ? "high" : "low");
                    };
                }
            }
        }
    }

    void onClose() override
    {
        audio::play_next_tone_progression();
        _io_panels.clear();
        for (const auto& io : _io_pins) {
            GetHAL()->gpioReset(io);
        }
        _mbus_window->close();
    }

private:
    std::vector<std::unique_ptr<GpioOutputTestPanel>> _io_panels;
    std::unique_ptr<Label> _label_msg;
    const std::array<uint8_t, 6> _io_pins = {49, 50, 0, 1, 54, 53};

    std::unique_ptr<ui::Window> _mbus_window;
};

void PanelGpioTest::init()
{
    _btn_gpio_test = std::make_unique<Container>(lv_screen_active());
    _btn_gpio_test->align(LV_ALIGN_CENTER, 413, -115);
    _btn_gpio_test->setSize(100, 100);
    _btn_gpio_test->setOpa(0);
    _btn_gpio_test->onClick().connect([&] {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<ExtPortWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });
}

void PanelGpioTest::update(bool isStacked)
{
    if (_window) {
        _window->update();
        if (_window->getState() == ui::Window::State_t::Closed) {
            _window.reset();
        }
    }
}
