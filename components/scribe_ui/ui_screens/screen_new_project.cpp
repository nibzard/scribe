#include "screen_new_project.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_NEW_PROJECT";

ScreenNewProject::ScreenNewProject() : screen_(nullptr) {
}

ScreenNewProject::~ScreenNewProject() {
    if (screen_) {
        lv_obj_del(screen_);
    }
}

void ScreenNewProject::init() {
    ESP_LOGI(TAG, "Initializing new project screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(screen_, LV_OPA_90, 0);

    createWidgets();
}

void ScreenNewProject::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, "New project");
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_20, 0);

    // Prompt
    prompt_label_ = lv_label_create(screen_);
    lv_label_set_text(prompt_label_, "Name your project");
    lv_obj_align(prompt_label_, LV_ALIGN_TOP_MID, 0, 100);

    // Name input
    name_input_ = lv_textarea_create(screen_);
    lv_obj_set_size(name_input_, 300, 50);
    lv_obj_align(name_input_, LV_ALIGN_TOP_MID, 0, 130);
    lv_textarea_set_placeholder_text(name_input_, "My Novel");
    lv_textarea_set_one_line(name_input_, true);
    lv_textarea_set_max_length(name_input_, 64);
    lv_obj_add_state(name_input_, LV_STATE_FOCUSED);

    // Error label (hidden initially)
    error_label_ = lv_label_create(screen_);
    lv_label_set_text(error_label_, "");
    lv_obj_align(error_label_, LV_ALIGN_TOP_MID, 0, 190);
    lv_obj_set_style_text_color(error_label_, lv_color_hex(0xFF0000), 0);
    lv_obj_add_flag(error_label_, LV_OBJ_FLAG_HIDDEN);

    // Create button
    create_btn_ = lv_btn_create(screen_);
    lv_obj_set_size(create_btn_, 140, 50);
    lv_obj_align(create_btn_, LV_ALIGN_CENTER, -80, 50);

    lv_obj_t* label = lv_label_create(create_btn_);
    lv_label_set_text(label, "Create");
    lv_obj_center(label);

    // Cancel button
    cancel_btn_ = lv_btn_create(screen_);
    lv_obj_set_size(cancel_btn_, 140, 50);
    lv_obj_align(cancel_btn_, LV_ALIGN_CENTER, 80, 50);

    label = lv_label_create(cancel_btn_);
    lv_label_set_text(label, "Cancel");
    lv_obj_center(label);

}

void ScreenNewProject::show() {
    if (screen_) {
        lv_scr_load(screen_);
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
        lv_textarea_del_char(name_input_);
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
