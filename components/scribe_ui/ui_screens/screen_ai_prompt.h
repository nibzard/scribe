#pragma once

#include <lvgl.h>
#include <string>

// AI prompt dialog - centered popup with a single input field
class ScreenAIPrompt {
public:
    ScreenAIPrompt();
    ~ScreenAIPrompt();

    void init();
    void show();
    void hide();

    bool isVisible() const { return visible_; }

    void clearPrompt();
    void appendPromptChar(char c);
    void backspacePrompt();
    std::string getPrompt() const;

private:
    lv_obj_t* dialog_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* prompt_input_ = nullptr;
    lv_obj_t* hint_label_ = nullptr;
    bool visible_ = false;

    void createWidgets();
};
