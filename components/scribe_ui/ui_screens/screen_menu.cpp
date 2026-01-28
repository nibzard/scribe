#include "screen_menu.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_MENU";

ScreenMenu::ScreenMenu() : screen_(nullptr), list_(nullptr), selected_index_(0) {
}

ScreenMenu::~ScreenMenu() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenMenu::init() {
    ESP_LOGI(TAG, "Initializing menu screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenMenu::createWidgets() {
    // Title
    lv_obj_t* title = lv_label_create(screen_);
    lv_label_set_text(title, Strings::getInstance().get("menu.title"));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    // Menu list
    list_ = lv_list_create(screen_);
    lv_obj_set_size(list_, 300, 400);
    lv_obj_center(list_);
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(list_, colors.fg, 0);
    lv_obj_set_style_bg_opa(list_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(list_, colors.border, 0);
    lv_obj_set_style_border_width(list_, 1, 0);
}

void ScreenMenu::setItems(const MenuItem* items, size_t count) {
    if (!list_) return;

    lv_obj_clean(list_);
    buttons_.clear();
    items_.clear();
    button_data_.clear();
    items_.reserve(count);
    buttons_.reserve(count);
    button_data_.reserve(count);
    const Theme::Colors& colors = Theme::getColors();

    for (size_t i = 0; i < count; i++) {
        items_.push_back(items[i]);
        lv_obj_t* btn = lv_list_add_button(list_, LV_SYMBOL_LIST, items[i].label);
        lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
        buttons_.push_back(btn);

        button_data_.push_back({this, static_cast<int>(i)});
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* target = lv_event_get_target_obj(e);
            ButtonData* data = static_cast<ButtonData*>(lv_obj_get_user_data(target));
            if (!data || !data->screen) {
                return;
            }
            data->screen->selected_index_ = data->index;
            data->screen->updateSelection();
            data->screen->selectCurrent();
        }, LV_EVENT_CLICKED, nullptr);
        lv_obj_set_user_data(btn, &button_data_.back());
    }

    selected_index_ = 0;
    updateSelection();
}

void ScreenMenu::show() {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        const Theme::Colors& colors = Theme::getColors();
        if (list_) {
            lv_obj_set_style_bg_color(list_, colors.fg, 0);
            lv_obj_set_style_bg_opa(list_, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(list_, colors.border, 0);
            lv_obj_set_style_border_width(list_, 1, 0);
        }
        for (auto* btn : buttons_) {
            if (!btn) continue;
            lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
        }
        lv_screen_load(screen_);
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
