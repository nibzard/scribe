#pragma once

#include <lvgl.h>
#include <functional>

// Diagnostics screen - System info, logs export, hardware test
class ScreenDiagnostics {
public:
    using CloseCallback = std::function<void()>;

    ScreenDiagnostics();
    ~ScreenDiagnostics();

    void init();
    void show();
    void hide();

    void setCloseCallback(CloseCallback cb) { close_cb_ = cb; }

    void refreshData();

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* info_list_ = nullptr;
    lv_obj_t* log_label_ = nullptr;

    CloseCallback close_cb_;

    void createWidgets();
    void updateSystemInfo();
    void exportLogs();
};
