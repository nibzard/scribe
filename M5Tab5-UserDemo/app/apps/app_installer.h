/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
#include <memory>
#include <hal/hal.h>
#include "app_template/app_template.h"
#include "app_launcher/app_launcher.h"
#include "app_startup_anim/app_startup_anim.h"
/* Header files locator (Don't remove) */

// Start boot anim app and wait for it to finish
inline void on_startup_anim()
{
    auto app_id = mooncake::GetMooncake().installApp(std::make_unique<AppStartupAnim>());
    mooncake::GetMooncake().openApp(app_id);
    while (1) {
        mooncake::GetMooncake().update();
        if (mooncake::GetMooncake().getAppCurrentState(app_id) == mooncake::AppAbility::StateSleeping) {
            break;
        }
        GetHAL()->delay(1);
    }
    mooncake::GetMooncake().uninstallApp(app_id);
}

/**
 * @brief App 安装回调
 *
 * @param mooncake
 */
inline void on_install_apps()
{
    // 安装 App
    // mooncake::GetMooncake().installApp(std::make_unique<AppTemplate>());
    mooncake::GetMooncake().installApp(std::make_unique<AppLauncher>());
    /* Install app locator (Don't remove) */
}
