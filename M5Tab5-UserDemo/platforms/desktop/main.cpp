/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal/hal_desktop.h"
#include <app.h>
#include <memory>
#include <hal/hal.h>

int main()
{
    // 应用层初始化回调
    app::InitCallback_t callback;

    callback.onHalInjection = []() {
        // 注入桌面平台的硬件抽象
        hal::Inject(std::make_unique<HalDesktop>());
    };

    // 启动应用层
    app::Init(callback);
    while (!app::IsDone()) {
        app::Update();
    }
    app::Destroy();

    return 0;
}
