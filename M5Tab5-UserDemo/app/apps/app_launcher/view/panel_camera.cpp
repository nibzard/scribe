/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-camera";

static const ui::Window::KeyFrame_t _kf_camera_close = {598, 0, 74, 74, 0};
static const ui::Window::KeyFrame_t _kf_camera_open  = {141, 0, 800, 480, 255};

class CameraWindow : public ui::Window {
public:
    CameraWindow()
    {
        config.kfClosed = _kf_camera_close;
        config.kfOpened = _kf_camera_open;
    }

    void close(bool teleport = false, bool triggerCallback = true) override
    {
        if (!_is_camera_opened) {
            ui::Window::close(teleport, triggerCallback);
            return;
        }

        if (!_is_camera_closing) {
            _is_camera_closing = true;
            _camera_canvas->setOpa(0);
            _label_msg->setText("Closing Camera ...");
            GetHAL()->stopCameraCapture();
        }
    }

    void onOpen() override
    {
        _window->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);

        _label_msg = std::make_unique<Label>(_window->get());
        _label_msg->align(LV_ALIGN_CENTER, 0, -24);
        _label_msg->setTextFont(&lv_font_montserrat_24);
        _label_msg->setText("Opening Camera ...");

        _camera_canvas = std::make_unique<Canvas>(lv_screen_active());
        _camera_canvas->setAlign(LV_ALIGN_CENTER);
        _camera_canvas->addFlag(LV_OBJ_FLAG_CLICKABLE);
        _camera_canvas->setOpa(0);
        _camera_canvas->onClick().connect([&]() {
            _is_camera_minimized = !_is_camera_minimized;
            update_camera_canvas();
        });

        update_camera_canvas();
    }

    void onUpdate() override
    {
        if (_is_camera_closing) {
            if (GetHAL()->isCameraCapturing()) {
                return;
            }
            _is_camera_closing = false;
            _label_msg->setText("Camera Closed");
            ui::Window::close();
        }

        if (_state != State_t::Opened) {
            return;
        }

        if (!_is_camera_opened) {
            GetHAL()->startCameraCapture(_camera_canvas->get());
            _is_camera_opened = true;
            _camera_canvas->setOpa(255);
        }
    }

    void onClose() override
    {
        audio::play_next_tone_progression();
    }

private:
    std::unique_ptr<Label> _label_msg;
    std::unique_ptr<Canvas> _camera_canvas;
    bool _is_camera_opened    = false;
    bool _is_camera_minimized = true;
    bool _is_camera_closing   = false;

    void update_camera_canvas()
    {
        if (_is_camera_minimized) {
            _camera_canvas->setPos(141, 0);
            _camera_canvas->setSize(760, 440);
            _camera_canvas->setRadius(12);
        } else {
            _camera_canvas->setPos(0, 0);
            _camera_canvas->setSize(1280, 720);
            _camera_canvas->setRadius(0);
        }
    }
};

void PanelCamera::init()
{
    _btn_camera = std::make_unique<Container>(lv_screen_active());
    _btn_camera->align(LV_ALIGN_CENTER, 598, 10);
    _btn_camera->setSize(83, 97);
    _btn_camera->setOpa(0);
    _btn_camera->onClick().connect([&] {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<CameraWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });
}

void PanelCamera::update(bool isStacked)
{
    if (_window) {
        _window->update();
        if (_window->getState() == ui::Window::State_t::Closed) {
            _window.reset();
        }
    }
}
