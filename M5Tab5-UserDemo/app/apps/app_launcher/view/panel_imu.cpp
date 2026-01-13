/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <algorithm>
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

static const std::string _tag = "panel-imu";

static constexpr int16_t _label_accel_x_pos_x = 1019;
static constexpr int16_t _label_accel_x_pos_y = 153;
static constexpr int16_t _label_accel_y_pos_x = 951;
static constexpr int16_t _label_accel_y_pos_y = 218;
static constexpr int16_t _label_accel_z_pos_x = 1019;
static constexpr int16_t _label_accel_z_pos_y = 245;
static constexpr int16_t _accel_dot_pos_x     = 371;
static constexpr int16_t _accel_dot_pos_y     = 198;
static constexpr uint32_t _label_color        = 0x606060;
static constexpr uint32_t _accel_dot_color    = 0xFF589D;

void PanelImu::init()
{
    _label_accel_x = std::make_unique<Label>(lv_screen_active());
    _label_accel_x->align(LV_ALIGN_LEFT_MID, _label_accel_x_pos_x, _label_accel_x_pos_y);
    _label_accel_x->setTextColor(lv_color_hex(_label_color));
    _label_accel_x->setTextFont(&lv_font_montserrat_16);
    _label_accel_x->setText("..");

    _label_accel_y = std::make_unique<Label>(lv_screen_active());
    _label_accel_y->align(LV_ALIGN_LEFT_MID, _label_accel_y_pos_x, _label_accel_y_pos_y);
    _label_accel_y->setTextColor(lv_color_hex(_label_color));
    _label_accel_y->setTextFont(&lv_font_montserrat_16);
    _label_accel_y->setText("..");

    _label_accel_z = std::make_unique<Label>(lv_screen_active());
    _label_accel_z->align(LV_ALIGN_LEFT_MID, _label_accel_z_pos_x, _label_accel_z_pos_y);
    _label_accel_z->setTextColor(lv_color_hex(_label_color));
    _label_accel_z->setTextFont(&lv_font_montserrat_16);
    _label_accel_z->setText("..");

    _accel_dot = std::make_unique<Container>(lv_screen_active());
    _accel_dot->align(LV_ALIGN_CENTER, _accel_dot_pos_x, _accel_dot_pos_y);
    _accel_dot->setSize(32, 32);
    _accel_dot->setRadius(233);
    _accel_dot->setBgColor(lv_color_hex(_accel_dot_color));
    _accel_dot->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    _accel_dot->setBorderWidth(0);

    _anim_x.pause();
    _anim_x.easingOptions().duration       = 0.1;
    _anim_x.easingOptions().easingFunction = ease::linear;
    _anim_x.teleport(_accel_dot_pos_x);
    _anim_x.play();

    _anim_y.pause();
    _anim_y.easingOptions().duration       = 0.1;
    _anim_y.easingOptions().easingFunction = ease::linear;
    _anim_y.teleport(_accel_dot_pos_y);
    _anim_y.play();

    _anim_size.pause();
    _anim_size.easingOptions().duration       = 0.3;
    _anim_size.easingOptions().easingFunction = ease::linear;
    _anim_size.teleport(22);
    _anim_size.play();
}

void PanelImu::update(bool isStacked)
{
    if (!(_anim_x.done() && _anim_y.done() && _anim_size.done())) {
        _accel_dot->setPos(_anim_x, _anim_y);
        _anim_size.update();
        _accel_dot->setSize(_anim_size.directValue(), _anim_size.directValue());
    }

    if (GetHAL()->millis() - _time_count < 100) {
        return;
    }

    GetHAL()->updateImuData();
    _label_accel_x->setText(fmt::format("X:{:.1f}", GetHAL()->imuData.accelX));
    _label_accel_y->setText(fmt::format("Y:{:.1f}", GetHAL()->imuData.accelY));
    _label_accel_z->setText(fmt::format("Z:{:.1f}", GetHAL()->imuData.accelZ));

    // Update dot position
    int dot_offset_x = std::clamp((int)(GetHAL()->imuData.accelX * 50), -50, 50);
    int dot_offset_y = std::clamp((int)(GetHAL()->imuData.accelY * 50), -50, 50);
    _anim_x          = _accel_dot_pos_x + dot_offset_x;
    _anim_y          = _accel_dot_pos_y + dot_offset_y;

    // Update dot size
    float gyro_mag = std::max(
        {std::abs(GetHAL()->imuData.gyroX), std::abs(GetHAL()->imuData.gyroY), std::abs(GetHAL()->imuData.gyroZ)});
    float accel_mag = std::max(
        {std::abs(GetHAL()->imuData.accelX), std::abs(GetHAL()->imuData.accelY), std::abs(GetHAL()->imuData.accelZ)});
    float motion_score = std::max(gyro_mag, accel_mag) * 10;

    // (10, 20) -> (22, 58)
    if (motion_score <= 10) {
        _anim_size = 22;
    } else if (motion_score >= 20) {
        _anim_size = 58;
    } else {
        _anim_size = 22 + (motion_score - 10) * (58 - 22) / (20 - 10);
    }

    _time_count = GetHAL()->millis();
}
