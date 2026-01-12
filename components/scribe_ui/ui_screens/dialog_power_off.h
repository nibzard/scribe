#pragma once

#include <lvgl.h>
#include <functional>

// Power off confirmation dialog
class DialogPowerOff {
public:
    using ConfirmCallback = std::function<void()>;
    using CancelCallback = std::function<void()>;

    DialogPowerOff();
    ~DialogPowerOff();

    void init();
    void show();
    void hide();

    void setConfirmCallback(ConfirmCallback cb) { confirm_cb_ = cb; }
    void setCancelCallback(CancelCallback cb) { cancel_cb_ = cb; }

private:
    lv_obj_t* dialog_ = nullptr;

    ConfirmCallback confirm_cb_;
    CancelCallback cancel_cb_;

    void createWidgets();
};
