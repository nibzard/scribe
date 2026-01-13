/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "shared.h"

static shared_data::SharedData_t* _shared_data_instance = nullptr;

shared_data::SharedData_t* shared_data::Get()
{
    if (!_shared_data_instance) {
        _shared_data_instance = new shared_data::SharedData_t;
    }
    return _shared_data_instance;
}

void shared_data::Destroy()
{
    if (_shared_data_instance) {
        delete _shared_data_instance;
        _shared_data_instance = nullptr;
    }
}
