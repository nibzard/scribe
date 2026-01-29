#pragma once

#include <lvgl.h>
#include <functional>
#include <string>
#include <vector>

class ScreenStorage {
public:
    ScreenStorage();
    ~ScreenStorage();

    void init();
    void show();
    void hide();

    void moveSelection(int delta);
    void selectCurrent();

    void setBackCallback(std::function<void()> cb) { back_cb_ = cb; }
    void setFormatCallback(std::function<void()> cb) { format_cb_ = cb; }

    void requestFormatPrompt() { prompt_on_show_ = true; }
    bool isConfirmVisible() const { return confirm_dialog_ != nullptr; }
    bool isBusy() const { return busy_; }
    void setBusy(bool busy, const char* status = nullptr, const char* detail = nullptr);
    void setStatusText(const char* status, const char* detail);
    void refreshStatus();
    void confirmFormat();
    void cancelConfirm();

private:
    lv_obj_t* screen_;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* list_;
    lv_obj_t* status_label_;
    lv_obj_t* detail_label_;
    lv_obj_t* confirm_dialog_{nullptr};
    lv_obj_t* confirm_body_{nullptr};
    lv_obj_t* busy_overlay_{nullptr};
    lv_obj_t* busy_label_{nullptr};
    lv_obj_t* busy_detail_{nullptr};
    lv_obj_t* busy_spinner_{nullptr};
    int selected_index_ = 0;
    bool prompt_on_show_ = false;
    int confirm_step_ = 0;
    bool busy_ = false;

    std::vector<lv_obj_t*> buttons_;
    std::vector<std::string> item_keys_;

    std::function<void()> back_cb_;
    std::function<void()> format_cb_;

    void createWidgets();
    void updateSelection();
    void updateStatus();
    void showConfirmDialog();
    void hideConfirmDialog();
    void updateBusyStyle();
};
