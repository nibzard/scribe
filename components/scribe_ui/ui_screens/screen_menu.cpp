#include "screen_menu.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_MENU";

// Default menu items from spec
static const MenuItem default_items[] = {
    {"Resume writing", []() {}},
    {"Switch project", []() {}},
    {"New project", []() {}},
    {"Find", []() {}},
    {"Export", []() {}},
    {"Settings", []() {}},
    {"Help", []() {}},
    {"Sleep", []() {}},
    {"Power off", []() {}},
};

ScreenMenu::ScreenMenu() : screen_(nullptr), list_(nullptr), selected_index_(0) {
}

ScreenMenu::~ScreenMenu() {
    if (screen_) {
        lv_obj_del(screen_);
    }
}

void ScreenMenu::init() {
    ESP_LOGI(TAG, "Initializing menu screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(screen_, LV_OPA_90, 0);

    createWidgets();
    setItems(default_items, sizeof(default_items) / sizeof(MenuItem));
}

void ScreenMenu::createWidgets() {
    // Title
    lv_obj_t* title = lv_label_create(screen_);
    lv_label_set_text(title, "Menu");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    // Menu list
    list_ = lv_list_create(screen_);
    lv_obj_set_size(list_, 300, 400);
    lv_obj_center(list_);
}

void ScreenMenu::setItems(const MenuItem* items, size_t count) {
    if (!list_) return;

    lv_obj_clean(list_);
    buttons_.clear();
    items_.clear();
    items_.reserve(count);
    buttons_.reserve(count);

    for (size_t i = 0; i < count; i++) {
        items_.push_back(items[i]);
        lv_obj_t* btn = lv_list_add_btn(list_, LV_SYMBOL_LIST, items[i].label);
        buttons_.push_back(btn);
    }

    selected_index_ = 0;
    updateSelection();
}

void ScreenMenu::show() {
    if (screen_) {
        lv_scr_load(screen_);
        selected_index_ = 0;
        updateSelection();
    }
}

void ScreenMenu::hide() {
    if (close_callback_) {
        close_callback_();
    }
}

void ScreenMenu::moveSelection(int delta) {
    int count = lv_obj_get_child_cnt(list_);
    selected_index_ += delta;

    if (selected_index_ < 0) selected_index_ = count - 1;
    if (selected_index_ >= count) selected_index_ = 0;

    updateSelection();
}

void ScreenMenu::selectCurrent() {
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(items_.size())) {
        return;
    }
    MenuItem& item = items_[selected_index_];
    if (item.action) {
        item.action();
    }
}

void ScreenMenu::updateSelection() {
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (static_cast<int>(i) == selected_index_) {
            lv_obj_add_state(buttons_[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(buttons_[i], LV_STATE_CHECKED);
        }
    }
    lv_obj_invalidate(list_);
}
