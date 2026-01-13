/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <queue>
#include <functional>
#include <smooth_ui_toolkit.h>

/**
 * @brief 共享数据层，提供一个带互斥锁的全局共享数据单例
 *
 */
namespace shared_data {

/**
 * @brief 共享数据定义
 *
 */
struct SharedData_t {
    smooth_ui_toolkit::Signal<std::string> systemStateEvents;
    smooth_ui_toolkit::Signal<std::string> inputEvents;
};

/**
 * @brief 获取共享数据实例
 *
 * @return SharedData_t&
 */
SharedData_t* Get();

/**
 * @brief 销毁共享数据实例
 *
 */
void Destroy();

}  // namespace shared_data

/**
 * @brief 获取共享数据实例
 *
 * @return SharedData_t&
 */
inline shared_data::SharedData_t* GetSharedData()
{
    return shared_data::Get();
}

inline smooth_ui_toolkit::Signal<std::string>& GetSystemStateEvents()
{
    return GetSharedData()->systemStateEvents;
}

inline smooth_ui_toolkit::Signal<std::string>& GetInputEvents()
{
    return GetSharedData()->inputEvents;
}
