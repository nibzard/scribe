/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
#include <memory>
#include "lvgl_cpp/label.h"
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <stdint.h>

/**
 * @brief 派生 App
 *
 */
class AppStartupAnim : public mooncake::AppAbility {
public:
    AppStartupAnim();

    // 重写生命周期回调
    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    enum AnimState_t {
        AnimState_StartupDelay = 0,
        AnimState_LogoTabMoveUp,
        AnimState_Logo5MoveLeft,
        AnimState_FinalDelay
    };

    AnimState_t _anime_state = AnimState_StartupDelay;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> _logo_tab;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Image> _logo_5;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _label_version;
    smooth_ui_toolkit::AnimateValue _anim_logo_tab_y;
    smooth_ui_toolkit::AnimateValue _anim_logo_tab_opa;
    smooth_ui_toolkit::AnimateValue _anim_logo_5_x;
    uint32_t _time_count = 0;
    bool _is_sfx_played  = false;
};
