#pragma once

#include <lvgl.h>
#include <string>
#include <functional>

// Recovery screen - Shown when autosave.tmp or journal is found
class ScreenRecovery {
public:
    using RecoveryAction = std::function<void(bool restore)>;

    ScreenRecovery();
    ~ScreenRecovery();

    void init();
    void show(const std::string& recovered_text);
    void hide();

    void setRecoveryCallback(RecoveryAction cb) { recovery_cb_ = cb; }
    void setSelectedRestore(bool restore);

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* icon_label_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* message_label_ = nullptr;
    lv_obj_t* preview_label_ = nullptr;
    lv_obj_t* restore_btn_ = nullptr;
    lv_obj_t* keep_btn_ = nullptr;

    RecoveryAction recovery_cb_;

    void createWidgets();
};
