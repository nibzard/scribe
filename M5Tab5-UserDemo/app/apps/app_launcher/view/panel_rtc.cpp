/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>
#include <apps/utils/ui/window.h>
#include <apps/utils/ui/toast.h>
#include <ctime>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-rtc";

static const ui::Window::KeyFrame_t _kf_rtc_setting_close = {436, -219, 75, 75, 0};
static const ui::Window::KeyFrame_t _kf_rtc_setting_open  = {214, 85, 800, 480, 255};

class RtcSettingWindow : public ui::Window {
public:
    RtcSettingWindow()
    {
        config.title    = "RTC Setting";
        config.kfClosed = _kf_rtc_setting_close;
        config.kfOpened = _kf_rtc_setting_open;
    }

    void onOpen() override
    {
        _window->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);

        _label_msg = std::make_unique<Label>(_window->get());
        _label_msg->align(LV_ALIGN_CENTER, 0, 0);
        _label_msg->setTextFont(&lv_font_montserrat_24);
        _label_msg->setTextColor(lv_color_hex(0xECEBEB));
        _label_msg->setText("Loading ...");
    }

    void onUpdate() override
    {
        if (_state == Opened) {
            if (!_calendar) {
                _label_msg.reset();

                std::time_t now     = std::time(nullptr);
                std::tm* local_time = std::localtime(&now);

                _calendar = std::make_unique<Calendar>(_window->get());
                _calendar->setSize(740, 330);
                _calendar->align(LV_ALIGN_CENTER, 0, -20);
                _calendar->setBorderWidth(0, LV_PART_MAIN);
                _calendar->setBgColor(lv_color_hex(config.bgColor));
                _calendar->headerDropdownCreate();
                _calendar->setTodayDate(local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday);
                _calendar->setShowedDate(local_time->tm_year + 1900, local_time->tm_mon + 1);
                _calendar->onValueChanged().connect(
                    [&](lv_calendar_date_t date) { _calendar->setTodayDate(date.year, date.month, date.day); });

                _btn_apply = std::make_unique<Button>(_window->get());
                _btn_apply->align(LV_ALIGN_CENTER, 280, 184);
                _btn_apply->setSize(155, 43);
                _btn_apply->label().setText("Apply");
                _btn_apply->label().setTextFont(&lv_font_montserrat_24);
                _btn_apply->setShadowWidth(0);
                _btn_apply->setRadius(18);
                _btn_apply->setBgColor(lv_color_hex(0xF26F42));
                _btn_apply->onClick().connect([&]() {
                    std::time_t now     = std::time(nullptr);
                    std::tm target_time = *std::localtime(&now);

                    target_time.tm_year = _calendar->getTodayDate()->year - 1900;
                    target_time.tm_mon  = _calendar->getTodayDate()->month - 1;
                    target_time.tm_mday = _calendar->getTodayDate()->day;
                    target_time.tm_hour = _roller_h->getSelected();
                    target_time.tm_min  = _roller_m->getSelected();
                    target_time.tm_sec  = _roller_s->getSelected();

                    ui::pop_a_toast(fmt::format("Set RTC time to {}/{}/{} {}:{:02d}:{:02d}", target_time.tm_year + 1900,
                                                target_time.tm_mon + 1, target_time.tm_mday, target_time.tm_hour,
                                                target_time.tm_min, target_time.tm_sec),
                                    ui::toast_type::success);

                    GetHAL()->setRtcTime(target_time);
                });

                _roller_h = std::make_unique<Roller>(_window->get());
                _roller_h->setOptions(generate_options(0, 23), LV_ROLLER_MODE_INFINITE);
                _roller_h->setSelected(local_time->tm_hour);
                apply_roller_style(_roller_h.get(), -290, 185);

                _roller_m = std::make_unique<Roller>(_window->get());
                _roller_m->setOptions(generate_options(0, 59), LV_ROLLER_MODE_INFINITE);
                _roller_m->setSelected(local_time->tm_min);
                apply_roller_style(_roller_m.get(), -110, 185);

                _roller_s = std::make_unique<Roller>(_window->get());
                _roller_s->setOptions(generate_options(0, 59), LV_ROLLER_MODE_INFINITE);
                _roller_s->setSelected(local_time->tm_sec);
                apply_roller_style(_roller_s.get(), 70, 185);

                _label_colon_a = std::make_unique<Label>(_window->get());
                _label_colon_a->setTextFont(&lv_font_montserrat_24);
                _label_colon_a->align(LV_ALIGN_CENTER, -200, 183);
                _label_colon_a->setText(":");

                _label_colon_b = std::make_unique<Label>(_window->get());
                _label_colon_b->setTextFont(&lv_font_montserrat_24);
                _label_colon_b->align(LV_ALIGN_CENTER, -20, 183);
                _label_colon_b->setText(":");
            }
        }
    }

    void onClose() override
    {
        audio::play_next_tone_progression();
        _calendar.reset();
    }

private:
    std::unique_ptr<Calendar> _calendar;
    std::unique_ptr<Roller> _roller_h;
    std::unique_ptr<Roller> _roller_m;
    std::unique_ptr<Roller> _roller_s;
    std::unique_ptr<Label> _label_colon_a;
    std::unique_ptr<Label> _label_colon_b;
    std::unique_ptr<Button> _btn_apply;
    std::unique_ptr<Label> _label_msg;

    std::vector<std::string> generate_options(int start, int end)
    {
        std::vector<std::string> options;
        for (int i = start; i <= end; i++) {
            options.push_back(fmt::format("{:02d}", i));
        }
        return options;
    }

    void apply_roller_style(Roller* roller, int16_t x, int16_t y)
    {
        roller->setVisibleRowCount(1);
        roller->align(LV_ALIGN_CENTER, x, y);
        roller->setSize(150, 44);
        roller->setTextFont(&lv_font_montserrat_24, LV_PART_MAIN);
        roller->setBgColor(lv_color_hex(config.bgColor), LV_PART_SELECTED);
        roller->setRadius(12, LV_PART_MAIN);
    }
};

void PanelRtc::init()
{
    _label_time = std::make_unique<Label>(lv_screen_active());
    _label_time->align(LV_ALIGN_CENTER, 335, -249);
    _label_time->setTextFont(&lv_font_montserrat_22);
    _label_time->setTextColor(lv_color_hex(0xD86037));
    _label_time->setText("..");

    _label_date = std::make_unique<Label>(lv_screen_active());
    _label_date->align(LV_ALIGN_CENTER, 335, -223);
    _label_date->setTextFont(&lv_font_montserrat_18);
    _label_date->setTextColor(lv_color_hex(0xD86037));
    _label_date->setText("..");

    _btn_rtc_setting = std::make_unique<Container>(lv_screen_active());
    _btn_rtc_setting->align(LV_ALIGN_CENTER, 422, -228);
    _btn_rtc_setting->setSize(103, 95);
    _btn_rtc_setting->setOpa(0);
    _btn_rtc_setting->onClick().connect([&] {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<RtcSettingWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });
}

void PanelRtc::update(bool isStacked)
{
    if (_window) {
        _window->update();
        if (_window->getState() == ui::Window::State_t::Closed) {
            _window.reset();
        }
    }

    if (GetHAL()->millis() - _time_count < 1000) {
        return;
    }

    std::time_t now    = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);

    _label_time->setText(fmt::format("{}:{:02d}:{:02d}", localTime->tm_hour, localTime->tm_min, localTime->tm_sec));
    _label_date->setText(fmt::format("{}/{}/{}", localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday));

    _time_count = GetHAL()->millis();
}
