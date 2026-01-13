/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
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
#include <assets/assets.h>
#include <stdint.h>
#include <vector>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag                      = "panel-i2c-scan";
static const std::string _toast_msg_ext_5v_enable  = " EXT 5V enabled";
static const std::string _toast_msg_ext_5v_disable = "EXT 5V disabled";

static const ui::Window::KeyFrame_t _kf_rtc_setting_close = {-505, 229, 75, 75, 0};
static const ui::Window::KeyFrame_t _kf_rtc_setting_open  = {-356, -12, 512, 356, 255};

class I2cScanWindow : public ui::Window {
public:
    I2cScanWindow()
    {
        config.kfClosed = _kf_rtc_setting_close;
        config.kfOpened = _kf_rtc_setting_open;
        config.bgColor  = 0x1A1A1A;
    }

    void onOpen() override
    {
        _window->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);

        _img_i2c_dev_chart = std::make_unique<Image>(_window->get());
        _img_i2c_dev_chart->setAlign(LV_ALIGN_CENTER);

        _btn_porta_scan = std::make_unique<Container>(_window->get());
        _btn_porta_scan->align(LV_ALIGN_CENTER, -100, -132);
        _btn_porta_scan->setSize(122, 52);
        _btn_porta_scan->setOpa(0);
        _btn_porta_scan->onClick().connect([&] {
            _is_scanning_internal = false;
            _scan_time_count      = 0;
            audio::play_tone_from_midi(60 + 24);
            update_i2c_dev_chart();
            update_btn_ext5v_on();
        });

        _btn_internal_scan = std::make_unique<Container>(_window->get());
        _btn_internal_scan->align(LV_ALIGN_CENTER, 100, -132);
        _btn_internal_scan->setSize(122, 52);
        _btn_internal_scan->setOpa(0);
        _btn_internal_scan->onClick().connect([&] {
            _is_scanning_internal = true;
            _scan_time_count      = 0;
            audio::play_tone_from_midi(63 + 24);
            update_i2c_dev_chart();
            update_btn_ext5v_on();
        });

        _btn_ext5v_on = std::make_unique<Image>(_window->get());
        _btn_ext5v_on->align(LV_ALIGN_CENTER, -212, -133);
        _btn_ext5v_on->setSrc(&porta_i2c_ext5v_on);
        _btn_ext5v_on->addFlag(LV_OBJ_FLAG_CLICKABLE);
        _btn_ext5v_on->onClick().connect([&] {
            GetHAL()->setExt5vEnable(!GetHAL()->getExt5vEnable());
            audio::play_tone_from_midi(GetHAL()->getExt5vEnable() ? (63 + 24) : (60 + 24));
            ui::pop_a_toast(GetHAL()->getExt5vEnable() ? _toast_msg_ext_5v_enable : _toast_msg_ext_5v_disable,
                            GetHAL()->getExt5vEnable() ? ui::toast_type::error : ui::toast_type::success);
            update_btn_ext5v_on();
        });

        update_i2c_dev_chart();
        update_btn_ext5v_on();

        GetHAL()->initPortAI2c();
    }

    void onUpdate() override
    {
        if (!(_state == Opening || _state == Opened)) {
            return;
        }

        if (GetHAL()->millis() - _scan_time_count < 300) {
            return;
        }

        // Scan and create labels
        std::vector<uint8_t> addrs;
        if (!_is_scanning_internal && !GetHAL()->getExt5vEnable()) {
            // Scan without ext 5v cause no response blocking
        } else {
            addrs = GetHAL()->i2cScan(_is_scanning_internal);
        }
        _label_addrs.clear();
        for (auto addr : addrs) {
            _label_addrs.push_back(std::make_unique<Label>(_window->get()));
            apply_addr_label_style(_label_addrs.back().get(), addr);
        }

        _scan_time_count = GetHAL()->millis();
    }

    void onClose() override
    {
        audio::play_next_tone_progression();
        _label_addrs.clear();
        _img_i2c_dev_chart.reset();
        GetHAL()->deinitPortAI2c();
    }

private:
    std::unique_ptr<Image> _img_i2c_dev_chart;
    std::unique_ptr<Container> _btn_porta_scan;
    std::unique_ptr<Container> _btn_internal_scan;
    std::unique_ptr<Image> _btn_ext5v_on;
    std::vector<std::unique_ptr<Label>> _label_addrs;
    bool _is_scanning_internal = true;
    uint32_t _scan_time_count  = 0;

    void update_i2c_dev_chart()
    {
        if (_is_scanning_internal) {
            _img_i2c_dev_chart->setSrc(&internal_i2c_dev_chart);
        } else {
            _img_i2c_dev_chart->setSrc(&porta_i2c_dev_chart);
        }
    }

    void apply_addr_label_style(Label* label, uint8_t addr)
    {
        int16_t row = addr >> 4;
        int16_t col = addr & 0x0F;

        int16_t x = 78 + col * 27 - 512 / 2;
        int16_t y = 136 + row * 27 - 356 / 2;

        label->align(LV_ALIGN_CENTER, x, y);
        label->setTextFont(&lv_font_montserrat_16);
        label->setTextColor(lv_color_hex(0x352B2A));
        label->setText(fmt::format("{:02X}", addr));
    }

    void update_btn_ext5v_on()
    {
        if (_is_scanning_internal) {
            _btn_ext5v_on->setOpa(0);
        } else {
            if (GetHAL()->getExt5vEnable()) {
                _btn_ext5v_on->setOpa(255);
                _btn_ext5v_on->setSrc(&porta_i2c_ext5v_on);
            } else {
                _btn_ext5v_on->setOpa(0);
            }
        }
    }
};

void PanelI2cScan::init()
{
    _btn_i2c_scan = std::make_unique<Container>(lv_screen_active());
    _btn_i2c_scan->align(LV_ALIGN_CENTER, -505, 229);
    _btn_i2c_scan->setSize(100, 100);
    _btn_i2c_scan->setOpa(0);
    _btn_i2c_scan->onClick().connect([&] {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<I2cScanWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });
}

void PanelI2cScan::update(bool isStacked)
{
    if (_window) {
        _window->update();
        if (_window->getState() == ui::Window::State_t::Closed) {
            _window.reset();
        }
    }
}
