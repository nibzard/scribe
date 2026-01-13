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
#include <assets/assets.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-power-monitor";

static constexpr int16_t _label_voltage_pos_x = -442;
static constexpr int16_t _label_voltage_pos_y = -335;
static constexpr int16_t _label_current_pos_x = -442;
static constexpr int16_t _label_current_pos_y = -303;
static constexpr uint32_t _label_color        = 0x333333;

void PanelPowerMonitor::init()
{
    _label_voltage = std::make_unique<Label>(lv_screen_active());
    _label_voltage->align(LV_ALIGN_RIGHT_MID, _label_voltage_pos_x, _label_voltage_pos_y);
    _label_voltage->setText("..");
    _label_voltage->setTextColor(lv_color_hex(_label_color));
    _label_voltage->setTextFont(&lv_font_montserrat_22);

    _label_current = std::make_unique<Label>(lv_screen_active());
    _label_current->align(LV_ALIGN_RIGHT_MID, _label_current_pos_x, _label_current_pos_y);
    _label_current->setText("..");
    _label_current->setTextColor(lv_color_hex(_label_color));
    _label_current->setTextFont(&lv_font_montserrat_22);

    _label_cpu_temp = std::make_unique<Label>(lv_screen_active());
    _label_cpu_temp->align(LV_ALIGN_CENTER, -25, 82);
    _label_cpu_temp->setText("..");
    _label_cpu_temp->setTextColor(lv_color_hex(0x535353));
    _label_cpu_temp->setTextFont(&lv_font_montserrat_18);

    _img_chg_arrow_up = std::make_unique<Image>(lv_screen_active());
    _img_chg_arrow_up->align(LV_ALIGN_CENTER, 286, -281);
    _img_chg_arrow_up->setSrc(&chg_arrow_up);

    _img_chg_arrow_down = std::make_unique<Image>(lv_screen_active());
    _img_chg_arrow_down->align(LV_ALIGN_CENTER, 216, -264);
    _img_chg_arrow_down->setSrc(&chg_arrow_down);
}

void PanelPowerMonitor::update(bool isStacked)
{
    if (GetHAL()->millis() - _pm_data_update_time_count > 100) {
        GetHAL()->updatePowerMonitorData();

        _label_voltage->setText(fmt::format("{:.2f}V", GetHAL()->powerMonitorData.busVoltage));
        _label_current->setText(fmt::format("{:.2f}A", GetHAL()->powerMonitorData.shuntCurrent));

        if (GetHAL()->powerMonitorData.shuntCurrent < 0) {
            _img_chg_arrow_up->setOpa(0);
            _img_chg_arrow_down->setOpa(0);
        } else {
            _img_chg_arrow_up->setOpa(255);
            _img_chg_arrow_down->setOpa(255);
        }

        _pm_data_update_time_count = GetHAL()->millis();
    }

    if (GetHAL()->millis() - _cpu_temp_update_time_count > 1000) {
        _label_cpu_temp->setText(fmt::format("{}", GetHAL()->getCpuTemp()));
        _cpu_temp_update_time_count = GetHAL()->millis();
    }
}
