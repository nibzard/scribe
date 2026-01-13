/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
#include <functional>

/**
 * @brief Application layer
 *
 */
namespace app {

struct InitCallback_t {
    std::function<void()> onHalInjection = nullptr;
};

/**
 * @brief
 *
 * @param callback
 */
void Init(InitCallback_t callback);

/**
 * @brief
 *
 */
void Update();

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool IsDone();

/**
 * @brief
 *
 */
void Destroy();

}  // namespace app
