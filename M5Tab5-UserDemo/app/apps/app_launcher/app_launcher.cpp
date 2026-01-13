/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_launcher.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>
#include <smooth_lvgl.h>
#include <assets/assets.h>

using namespace mooncake;

AppLauncher::AppLauncher()
{
    setAppInfo().name = "AppLauncher";
}

void AppLauncher::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");

    open();
}

void AppLauncher::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    _view = std::make_unique<launcher_view::LauncherView>();
    _view->init();
}

void AppLauncher::onRunning()
{
    _view->update();
}

void AppLauncher::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    _view.reset();
}
