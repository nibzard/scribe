/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "window.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>

using namespace ui;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static smooth_ui_toolkit::Signal<bool> _signal_window_opened;  // true for opened, false for closed

smooth_ui_toolkit::Signal<bool>& ui::signal_window_opened()
{
    return _signal_window_opened;
}

void Window::init(lv_obj_t* parent)
{
    _window = std::make_unique<Container>(parent);
    _window->setAlign(LV_ALIGN_CENTER);
    _window->setPadding(0, 0, 0, 0);
    _window->setRadius(24);
    _window->setBorderWidth(1);
    _window->setBorderColor(lv_color_hex(config.borderColor));
    _window->setBgColor(lv_color_hex(config.bgColor));

    if (!config.title.empty()) {
        _title_label = std::make_unique<Label>(_window->get());
        _title_label->align(LV_ALIGN_TOP_MID, 0, 15);
        _title_label->setTextColor(lv_color_hex(config.titleColor));
        _title_label->setTextFont(&lv_font_montserrat_18);
        _title_label->setText(config.title);
        _title_label->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    }

    if (config.closeBtn) {
        _close_dot = std::make_unique<Container>(_window->get());
        _close_dot->setSize(20, 20);
        _close_dot->align(LV_ALIGN_TOP_RIGHT, -20, 16);
        _close_dot->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _close_dot->setRadius(10);
        _close_dot->setBgColor(lv_color_hex(config.closeDotColor));
        _close_dot->setBorderWidth(0);
        _close_dot->removeFlag(LV_OBJ_FLAG_CLICKABLE);

        _close_btn = std::make_unique<Container>(parent);
        _close_btn->setSize(80, 80);
        _close_btn->align(LV_ALIGN_CENTER, config.kfOpened.x + config.kfOpened.w / 2 - 30,
                          config.kfOpened.y - config.kfOpened.h / 2 + 26);
        _close_btn->setOpa(0);
        _close_btn->onClick().connect([&]() { close(); });
    }

    if (config.clickBgClose) {
        _bg_close_area = std::make_unique<Container>(parent);
        _bg_close_area->setSize(lv_obj_get_width(parent), lv_obj_get_height(parent));
        _bg_close_area->align(LV_ALIGN_CENTER, 0, 0);
        _bg_close_area->setOpa(0);
        _bg_close_area->onClick().connect([&]() { close(); });
    }

    // Sort layer
    _window->moveForeground();
    if (_close_btn) {
        _close_btn->moveForeground();
    }

    _anim_x.springOptions().visualDuration   = 0.4;
    _anim_x.springOptions().bounce           = 0.2;
    _anim_y.springOptions().visualDuration   = 0.4;
    _anim_y.springOptions().bounce           = 0.2;
    _anim_w.springOptions().visualDuration   = 0.4;
    _anim_w.springOptions().bounce           = 0.2;
    _anim_h.springOptions().visualDuration   = 0.4;
    _anim_h.springOptions().bounce           = 0.2;
    _anim_opa.easingOptions().easingFunction = ease::linear;

    onInit();

    close(true, false);
}

void Window::update()
{
    // Apply animation
    if (!(_anim_x.done() && _anim_y.done())) {
        _window->setPos(_anim_x, _anim_y);
    }
    if (!(_anim_w.done() && _anim_h.done())) {
        _window->setSize(_anim_w, _anim_h);
    }
    if (!_anim_opa.done()) {
        _window->setOpa(_anim_opa);
    }

    // Update state
    if (_anim_x.done() && _anim_y.done() && _anim_w.done() && _anim_h.done() && _anim_opa.done()) {
        if (_state == Opening) {
            _state = Opened;
        } else if (_state == Closing) {
            _state = Closed;
        }
    }

    onUpdate();
}

void Window::close(bool teleport, bool triggerCallback)
{
    _state = Closing;

    if (_close_btn) {
        _close_btn->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    }
    if (_bg_close_area) {
        _bg_close_area->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    }
    _window->removeFlag(LV_OBJ_FLAG_CLICKABLE);

    _anim_opa.easingOptions().duration = 0.8;
    update_anim(config.kfClosed, teleport);

    signal_window_opened().emit(false);

    if (triggerCallback) {
        onClose();
    }
}

void Window::open(bool teleport, bool triggerCallback)
{
    _state = Opening;

    if (_close_btn) {
        _close_btn->addFlag(LV_OBJ_FLAG_CLICKABLE);
    }
    if (_bg_close_area) {
        _bg_close_area->addFlag(LV_OBJ_FLAG_CLICKABLE);
    }
    _window->addFlag(LV_OBJ_FLAG_CLICKABLE);

    _anim_opa.easingOptions().duration = 0;
    update_anim(config.kfOpened, teleport);

    signal_window_opened().emit(true);

    if (triggerCallback) {
        onOpen();
    }
}

void Window::update_anim(const KeyFrame_t& target, bool teleport)
{
    if (teleport) {
        _anim_x.teleport(target.x);
        _anim_y.teleport(target.y);
        _anim_w.teleport(target.w);
        _anim_h.teleport(target.h);
        _anim_opa.teleport(target.opa);

        _window->setPos(_anim_x, _anim_y);
        _window->setSize(_anim_w, _anim_h);
        _window->setOpa(_anim_opa);
    } else {
        _anim_x   = target.x;
        _anim_y   = target.y;
        _anim_w   = target.w;
        _anim_h   = target.h;
        _anim_opa = target.opa;
    }
}
