/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app.h"
#include "hal/hal.h"
#include "apps/app_installer.h"
#include <mooncake.h>
#include <mooncake_log.h>
#include <string>
#include <thread>

using namespace mooncake;

static const std::string _tag = "app";

void app::Init(InitCallback_t callback)
{
    mclog::tagInfo(_tag, "init");

    mclog::tagInfo(_tag, "hal injection");
    if (callback.onHalInjection) {
        callback.onHalInjection();
    }

    GetMooncake();

    on_startup_anim();
    on_install_apps();
}

void app::Update()
{
    GetMooncake().update();

#if defined(__APPLE__) && defined(__MACH__)
    // 'nextEventMatchingMask should only be called from the Main Thread!'
    auto time_till_next = lv_timer_handler();
    std::this_thread::sleep_for(std::chrono::milliseconds(time_till_next));
#endif
}

bool app::IsDone()
{
    return false;
}

void app::Destroy()
{
    DestroyMooncake();
    hal::Destroy();
}
