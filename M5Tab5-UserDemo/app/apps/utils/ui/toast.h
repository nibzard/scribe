/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
// Ref: https://www.heroui.com/docs/components/toast
#include <cstdint>
#include <string>
#include <lvgl.h>
#include <mooncake.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <memory>
#include <vector>

namespace ui {

namespace toast_type {
enum Type_t {
    info = 0,
    warning,
    error,
    success,
    orange,
    gray,
    rose,
    dark,
};
}

class Toast {
public:
    enum State_t {
        Closed,
        Opening,
        Opened,
        Closing,
    };

    struct KeyFrame_t {
        int16_t y = 0;
        int16_t w = 0;
    };

    struct Config_t {
        toast_type::Type_t type = toast_type::info;
        uint32_t durationMs     = 1000;
        std::string msg;
    };

    Config_t config;

    void init(lv_obj_t* parent);
    void update();
    void close(bool teleport = false);
    void open(bool teleport = false);
    void stack(bool teleport = false);
    inline smooth_ui_toolkit::lvgl_cpp::Container* get()
    {
        return _toast.get();
    }
    inline State_t getState() const
    {
        return _state;
    }

protected:
    uint32_t _time_count = 0;
    State_t _state       = Closed;
    uint8_t _stack_depth = 0;

    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _toast;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _msg_label;

    smooth_ui_toolkit::AnimateValue _anim_y;
    smooth_ui_toolkit::AnimateValue _anim_w;

    void update_anim(const KeyFrame_t& target, bool teleport);
};

class ToastManager : public mooncake::BasicAbility {
public:
    void onCreate() override;
    void onRunning() override;

protected:
    int _current_toast_index = 0;
    std::vector<std::unique_ptr<Toast>> _toast_list;
};

void pop_a_toast(std::string msg, toast_type::Type_t type = toast_type::info, uint32_t durationMs = 1600);

}  // namespace ui
