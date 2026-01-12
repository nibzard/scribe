#pragma once

#include <lvgl.h>
#include <string>
#include <functional>

// New project dialog - Creates a new project with name input
class ScreenNewProject {
public:
    using CreateCallback = std::function<void(const std::string& name)>;
    using CancelCallback = std::function<void()>;

    ScreenNewProject();
    ~ScreenNewProject();

    void init();
    void show();
    void hide();

    void setCreateCallback(CreateCallback cb) { create_cb_ = cb; }
    void setCancelCallback(CancelCallback cb) { cancel_cb_ = cb; }

    std::string getName() const;
    void appendNameChar(char c);
    void backspaceName();
    void clearError();
    void showError(const char* message);

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* prompt_label_ = nullptr;
    lv_obj_t* name_input_ = nullptr;
    lv_obj_t* create_btn_ = nullptr;
    lv_obj_t* cancel_btn_ = nullptr;
    lv_obj_t* error_label_ = nullptr;

    CreateCallback create_cb_;
    CancelCallback cancel_cb_;

    void createWidgets();
};
