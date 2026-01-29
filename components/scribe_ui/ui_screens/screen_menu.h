#pragma once

#include <lvgl.h>
#include <functional>
#include <vector>

// Menu item
struct MenuItem {
    const char* label;
    std::function<void()> action;
};

// Esc menu screen - centered menu with keyboard navigation
class ScreenMenu {
public:
    ScreenMenu();
    ~ScreenMenu();

    void init();
    void show();
    void hide();

    // Set menu items
    void setItems(const MenuItem* items, size_t count);

    // Navigate menu
    void moveSelection(int delta);
    void selectCurrent();

    // Set callbacks
    void setCloseCallback(std::function<void()> cb) { close_callback_ = cb; }

private:
    struct ButtonData {
        ScreenMenu* screen;
        int index;
    };

    lv_obj_t* screen_;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* list_;
    int selected_index_ = 0;
    std::function<void()> close_callback_;
    std::vector<MenuItem> items_;
    std::vector<lv_obj_t*> buttons_;
    std::vector<ButtonData> button_data_;

    void createWidgets();
    void updateSelection();
    void sizeListToContent();
};
