#include "screen_help.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
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
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenHelp::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("help.title"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(20));
    lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Display), 0);

    // Help content in a scrollable area
    content_ = lv_obj_create(screen_);
    lv_obj_set_size(content_, LV_HOR_RES - Theme::scalePx(40), LV_VER_RES - Theme::scalePx(100));
    lv_obj_align(content_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(60));
    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_text_font(content_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Section: How to write
    lv_obj_t* label = lv_label_create(content_);
    lv_label_set_text(label, Strings::getInstance().get("help.how_to_write_title"));
    lv_obj_set_style_text_font(label, Theme::getUIFont(Theme::UiFontRole::Title), 0);
    heading_labels_.push_back(label);

    label = lv_label_create(content_);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - Theme::scalePx(60));
    lv_label_set_text(label, Strings::getInstance().get("help.how_to_write_body"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Section: Shortcuts
    label = lv_label_create(content_);
    lv_label_set_text(label, Strings::getInstance().get("help.shortcuts_title"));
    lv_obj_set_style_text_font(label, Theme::getUIFont(Theme::UiFontRole::Title), 0);
    heading_labels_.push_back(label);

    label = lv_label_create(content_);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - Theme::scalePx(60));
    lv_label_set_text(label, Strings::getInstance().get("help.shortcuts_body"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Section: Saving
    label = lv_label_create(content_);
    lv_label_set_text(label, Strings::getInstance().get("help.saving_title"));
    lv_obj_set_style_text_font(label, Theme::getUIFont(Theme::UiFontRole::Title), 0);
    heading_labels_.push_back(label);

    label = lv_label_create(content_);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - Theme::scalePx(60));
    lv_label_set_text(label, Strings::getInstance().get("help.saving_body"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Section: Export
    label = lv_label_create(content_);
    lv_label_set_text(label, Strings::getInstance().get("help.export_title"));
    lv_obj_set_style_text_font(label, Theme::getUIFont(Theme::UiFontRole::Title), 0);
    heading_labels_.push_back(label);

    label = lv_label_create(content_);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - Theme::scalePx(60));
    lv_label_set_text(label, Strings::getInstance().get("help.export_body"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Close instruction
    close_label_ = lv_label_create(screen_);
    lv_label_set_text(close_label_, Strings::getInstance().get("help.close"));
    lv_obj_align(close_label_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(10));
    lv_obj_set_style_text_font(close_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
}

void ScreenHelp::show() {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        const Theme::Colors& colors = Theme::getColors();
        lv_obj_set_style_text_font(screen_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        if (title_label_) {
            lv_obj_set_style_text_color(title_label_, colors.text, 0);
            lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Display), 0);
            lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(20));
        }
        if (content_) {
            lv_obj_set_size(content_, LV_HOR_RES - Theme::scalePx(40), LV_VER_RES - Theme::scalePx(100));
            lv_obj_align(content_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(60));
            lv_obj_set_style_text_font(content_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_set_style_bg_color(content_, colors.fg, 0);
            lv_obj_set_style_border_color(content_, colors.border, 0);
        }
        for (auto* heading : heading_labels_) {
            if (heading) {
                lv_obj_set_style_text_color(heading, colors.text, 0);
                lv_obj_set_style_text_font(heading, Theme::getUIFont(Theme::UiFontRole::Title), 0);
            }
        }
        if (close_label_) {
            lv_obj_set_style_text_color(close_label_, colors.text_secondary, 0);
            lv_obj_set_style_text_font(close_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
            lv_obj_align(close_label_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(10));
        }
        lv_screen_load(screen_);
    }
}

void ScreenHelp::hide() {
    if (close_cb_) {
        close_cb_();
    }
}
