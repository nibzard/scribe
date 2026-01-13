/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "view/view.h"
#include <mooncake.h>
#include <memory>

/**
 * @brief 派生 App
 *
 */
class AppLauncher : public mooncake::AppAbility {
public:
    AppLauncher();

    // 重写生命周期回调
    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    std::unique_ptr<launcher_view::LauncherView> _view;
};
