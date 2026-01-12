#include "screen_help.h"
#include "../../scribe_utils/strings.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_HELP";

ScreenHelp::ScreenHelp() : screen_(nullptr) {
}

ScreenHelp::~ScreenHelp() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenHelp::init() {
    ESP_LOGI(TAG, "Initializing help screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenHelp::createWidgets() {
    // Title
    lv_obj_t* title = lv_label_create(screen_);
    lv_label_set_text(title, Strings::getInstance().get("help.title"));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    // Help content in a scrollable area
    lv_obj_t* content = lv_obj_create(screen_);
    lv_obj_set_size(content, LV_HOR_RES - 40, LV_VER_RES - 100);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    // Section: How to write
    lv_obj_t* label = lv_label_create(content);
    lv_label_set_text(label, Strings::getInstance().get("help.how_to_write_title"));
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

    label = lv_label_create(content);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - 60);
    lv_label_set_text(label, Strings::getInstance().get("help.how_to_write_body"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Section: Shortcuts
    label = lv_label_create(content);
    lv_label_set_text(label, Strings::getInstance().get("help.shortcuts_title"));
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

    label = lv_label_create(content);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - 60);
    lv_label_set_text(label, Strings::getInstance().get("help.shortcuts_body"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Section: Saving
    label = lv_label_create(content);
    lv_label_set_text(label, Strings::getInstance().get("help.saving_title"));
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

    label = lv_label_create(content);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - 60);
    lv_label_set_text(label, Strings::getInstance().get("help.saving_body"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Section: Export
    label = lv_label_create(content);
    lv_label_set_text(label, Strings::getInstance().get("help.export_title"));
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

    label = lv_label_create(content);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - 60);
    lv_label_set_text(label, Strings::getInstance().get("help.export_body"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Close instruction
    label = lv_label_create(screen_);
    lv_label_set_text(label, Strings::getInstance().get("help.close"));
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void ScreenHelp::show() {
    if (screen_) {
        lv_screen_load(screen_);
    }
}

void ScreenHelp::hide() {
    if (close_cb_) {
        close_cb_();
    }
}
