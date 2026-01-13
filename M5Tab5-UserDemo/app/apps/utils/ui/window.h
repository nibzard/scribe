/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <lvgl.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <memory>
#include <string>
#include <cstdint>

namespace ui {

class Window {
public:
    virtual ~Window()
    {
    }

    enum State_t {
        Closed,
        Opening,
        Opened,
        Closing,
    };

    struct KeyFrame_t {
        int16_t x   = 0;
        int16_t y   = 0;
        int16_t w   = 800;
        int16_t h   = 480;
        int16_t opa = 255;
    };

    struct Config_t {
        KeyFrame_t kfClosed;
        KeyFrame_t kfOpened;
        std::string title;
        bool closeBtn          = false;
        bool clickBgClose      = true;
        uint32_t titleColor    = 0xD8D8D8;
        uint32_t closeDotColor = 0xFF5F57;
        uint32_t borderColor   = 0x000000;
        uint32_t bgColor       = 0x232323;
    };

    Config_t config;

    virtual void init(lv_obj_t* parent);
    virtual void update();
    virtual void close(bool teleport = false, bool triggerCallback = true);
    virtual void open(bool teleport = false, bool triggerCallback = true);
    inline smooth_ui_toolkit::lvgl_cpp::Container* get()
    {
        return _window.get();
    }
    inline State_t getState() const
    {
        return _state;
    }

    virtual void onInit()
    {
    }
    virtual void onOpen()
    {
    }
    virtual void onClose()
    {
    }
    virtual void onUpdate()
    {
    }

protected:
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _window;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _close_dot;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _close_btn;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _title_label;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _bg_close_area;

    smooth_ui_toolkit::AnimateValue _anim_x;
    smooth_ui_toolkit::AnimateValue _anim_y;
    smooth_ui_toolkit::AnimateValue _anim_w;
    smooth_ui_toolkit::AnimateValue _anim_h;
    smooth_ui_toolkit::AnimateValue _anim_opa;

    State_t _state = Closed;

    void update_anim(const KeyFrame_t& target, bool teleport);
};

/**
 * @brief Signal for window opened, true for opened, false for closed
 *
 * @return smooth_ui_toolkit::Signal<bool>&
 */
smooth_ui_toolkit::Signal<bool>& signal_window_opened();

}  // namespace ui
