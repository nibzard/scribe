#include "screen_new_project.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_NEW_PROJECT";

ScreenNewProject::ScreenNewProject() : screen_(nullptr) {
}

ScreenNewProject::~ScreenNewProject() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenNewProject::init() {
    ESP_LOGI(TAG, "Initializing new project screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenNewProject::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("new_project.title"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_20, 0);

    // Prompt
    prompt_label_ = lv_label_create(screen_);
    lv_label_set_text(prompt_label_, Strings::getInstance().get("new_project.prompt"));
    lv_obj_align(prompt_label_, LV_ALIGN_TOP_MID, 0, 100);

    // Name input
    name_input_ = lv_textarea_create(screen_);
    lv_obj_set_size(name_input_, 300, 50);
    lv_obj_align(name_input_, LV_ALIGN_TOP_MID, 0, 130);
    lv_textarea_set_placeholder_text(name_input_, Strings::getInstance().get("new_project.placeholder"));
    lv_textarea_set_one_line(name_input_, true);
    lv_textarea_set_max_length(name_input_, 64);
    lv_obj_add_state(name_input_, LV_STATE_FOCUSED);
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(name_input_, colors.fg, 0);
    lv_obj_set_style_bg_opa(name_input_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(name_input_, colors.border, 0);
    lv_obj_set_style_border_width(name_input_, 1, 0);
    lv_obj_set_style_text_color(name_input_, colors.text, 0);
    lv_obj_set_style_text_color(name_input_, colors.text_secondary, LV_PART_TEXTAREA_PLACEHOLDER);

    // Error label (hidden initially)
    error_label_ = lv_label_create(screen_);
    lv_label_set_text(error_label_, "");
    lv_obj_align(error_label_, LV_ALIGN_TOP_MID, 0, 190);
    lv_obj_set_style_text_color(error_label_, colors.error, 0);
    lv_obj_add_flag(error_label_, LV_OBJ_FLAG_HIDDEN);

    // Create button
    create_btn_ = lv_button_create(screen_);
    lv_obj_set_size(create_btn_, 140, 50);
    lv_obj_align(create_btn_, LV_ALIGN_CENTER, -80, 50);

    lv_obj_t* label = lv_label_create(create_btn_);
    lv_label_set_text(label, Strings::getInstance().get("new_project.create"));
    lv_obj_center(label);

    // Cancel button
    cancel_btn_ = lv_button_create(screen_);
    lv_obj_set_size(cancel_btn_, 140, 50);
    lv_obj_align(cancel_btn_, LV_ALIGN_CENTER, 80, 50);

    label = lv_label_create(cancel_btn_);
    lv_label_set_text(label, Strings::getInstance().get("new_project.cancel"));
    lv_obj_center(label);

}

void ScreenNewProject::show() {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        const Theme::Colors& colors = Theme::getColors();
        if (name_input_) {
            lv_obj_set_style_bg_color(name_input_, colors.fg, 0);
            lv_obj_set_style_bg_opa(name_input_, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(name_input_, colors.border, 0);
            lv_obj_set_style_border_width(name_input_, 1, 0);
            lv_obj_set_style_text_color(name_input_, colors.text, 0);
            lv_obj_set_style_text_color(name_input_, colors.text_secondary, LV_PART_TEXTAREA_PLACEHOLDER);
        }
        if (error_label_) {
            lv_obj_set_style_text_color(error_label_, colors.error, 0);
        }
        lv_screen_load(screen_);
        // Clear any previous input
        if (name_input_) {
            lv_textarea_set_text(name_input_, "");
            lv_obj_add_state(name_input_, LV_STATE_FOCUSED);
        }
        // Hide error
        if (error_label_) {
            lv_obj_add_flag(error_label_, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ScreenNewProject::hide() {
    if (cancel_cb_) {
        cancel_cb_();
    }
}

std::string ScreenNewProject::getName() const {
    if (!name_input_) return "";
    const char* text = lv_textarea_get_text(name_input_);
    return std::string(text ? text : "");
}

void ScreenNewProject::appendNameChar(char c) {
    if (!name_input_) return;
    char buf[2] = {c, 0};
    lv_textarea_add_text(name_input_, buf);
}

void ScreenNewProject::backspaceName() {
    if (name_input_) {
        lv_textarea_delete_char(name_input_);
    }
}

void ScreenNewProject::clearError() {
    if (error_label_) {
        lv_label_set_text(error_label_, "");
        lv_obj_add_flag(error_label_, LV_OBJ_FLAG_HIDDEN);
    }
}

void ScreenNewProject::showError(const char* message) {
    if (error_label_) {
        lv_label_set_text(error_label_, message);
        lv_obj_clear_flag(error_label_, LV_OBJ_FLAG_HIDDEN);
    }
}
