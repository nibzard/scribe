/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "lvgl_cpp/button.h"
#include "lvgl_cpp/label.h"
#include "view.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <memory>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>
#include <apps/utils/ui/window.h>
#include <apps/utils/ui/toast.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-com";

static const ui::Window::KeyFrame_t _kf_com_monitor_close = {497, 5, 75, 75, 0};
static const ui::Window::KeyFrame_t _kf_com_monitor_open  = {60, 0, 643, 435, 255};

class ComMonitorWindow : public ui::Window {
public:
    ComMonitorWindow()
    {
        config.kfClosed = _kf_com_monitor_close;
        config.kfOpened = _kf_com_monitor_open;
    }

    void onOpen() override
    {
        _window->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);

        _msg_panel = std::make_unique<TextArea>(_window->get());
        _msg_panel->setSize(600, 333);
        _msg_panel->align(LV_ALIGN_CENTER, 0, -33);
        _msg_panel->setMaxLength(4096);
        _msg_panel->setCursorClickPos(false);
        _msg_panel->setText("");
        _msg_panel->setPasswordMode(false);
        _msg_panel->setOneLine(false);
        _msg_panel->setBorderWidth(0);
        _msg_panel->setBgColor(lv_color_hex(0x383838));
        _msg_panel->setTextFont(&lv_font_montserrat_18);

        _label_msg = std::make_unique<Label>(_window->get());
        _label_msg->align(LV_ALIGN_LEFT_MID, 46, 172);
        _label_msg->setText("Port: RS485\nBaud: 115200");
        _label_msg->setTextFont(&lv_font_montserrat_18);
        _label_msg->setTextColor(lv_color_hex(0xDEDEDE));

        _btn_send_msg = std::make_unique<Button>(_window->get());
        _btn_send_msg->setSize(314, 48);
        _btn_send_msg->align(LV_ALIGN_CENTER, 128, 174);
        _btn_send_msg->setBgColor(lv_color_hex(0x616161));
        _btn_send_msg->setRadius(18);
        _btn_send_msg->label().setTextFont(&lv_font_montserrat_22);
        _btn_send_msg->label().setTextColor(lv_color_hex(0xE7E7E7));
        _btn_send_msg->label().setText("Send \"Hello M5Stack!\"");
        _btn_send_msg->onClick().connect([&] {
            audio::play_next_tone_progression();
            _msg_panel->addText("<<< Hello M5Stack!\n");
            GetHAL()->uartMonitorSend("Hello M5Stack!");
        });
    }

    void onUpdate() override
    {
        if (_state != Opened) {
            return;
        }

        std::lock_guard<std::mutex> lock(GetHAL()->uartMonitorData.mutex);
        while (!GetHAL()->uartMonitorData.rxQueue.empty()) {
            uint8_t c = GetHAL()->uartMonitorData.rxQueue.front();
            GetHAL()->uartMonitorData.rxQueue.pop();
            _msg_panel->addChar(c);
        }
    }

    void onClose() override
    {
        audio::play_next_tone_progression();
        _msg_panel.reset();
        _label_msg.reset();
        _btn_send_msg.reset();
    }

private:
    std::unique_ptr<TextArea> _msg_panel;
    std::unique_ptr<Label> _label_msg;
    std::unique_ptr<Button> _btn_send_msg;
};

void PanelComMonitor::init()
{
    _btn_com_monitor = std::make_unique<Container>(lv_screen_active());
    _btn_com_monitor->align(LV_ALIGN_CENTER, 497, 5);
    _btn_com_monitor->setSize(100, 100);
    _btn_com_monitor->setOpa(0);
    _btn_com_monitor->onClick().connect([&] {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<ComMonitorWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });
}

void PanelComMonitor::update(bool isStacked)
{
    if (_window) {
        _window->update();
        if (_window->getState() == ui::Window::State_t::Closed) {
            _window.reset();
        }
    }
}
