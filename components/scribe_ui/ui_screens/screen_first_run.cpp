#include "screen_first_run.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_FIRST_RUN";

ScreenFirstRun::ScreenFirstRun() : screen_(nullptr) {
}

ScreenFirstRun::~ScreenFirstRun() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenFirstRun::init() {
    ESP_LOGI(TAG, "Initializing first run screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenFirstRun::createWidgets() {
    const Theme::Colors& colors = Theme::getColors();
    // Welcome title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("app.name"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(40));
    lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Display), 0);

    // Tagline
    tagline_label_ = lv_label_create(screen_);
    lv_label_set_text(tagline_label_, Strings::getInstance().get("boot.tagline"));
    lv_obj_align(tagline_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(80));
    lv_obj_set_style_text_font(tagline_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);

    // Main tip in a scrollable area
    content_ = lv_obj_create(screen_);
    lv_obj_set_size(content_, LV_HOR_RES - Theme::scalePx(60), Theme::fitHeight(250, 200));
    lv_obj_align(content_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(120));
    lv_obj_set_style_bg_color(content_, colors.fg, 0);
    lv_obj_set_style_bg_opa(content_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(content_, colors.border, 0);
    lv_obj_set_style_border_width(content_, 1, 0);
    lv_obj_set_style_text_font(content_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    tip_label_ = lv_label_create(content_);
    lv_label_set_long_mode(tip_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(tip_label_, LV_HOR_RES - Theme::scalePx(80));
    lv_label_set_text(tip_label_, Strings::getInstance().get("first_run.tip"));
    lv_obj_set_style_text_align(tip_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(tip_label_);

    // Dismiss instruction
    dismiss_label_ = lv_label_create(screen_);
    lv_label_set_text(dismiss_label_, Strings::getInstance().get("first_run.dismiss"));
    lv_obj_align(dismiss_label_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(30));
    lv_obj_set_style_text_font(dismiss_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
}

void ScreenFirstRun::show() {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        const Theme::Colors& colors = Theme::getColors();
        if (title_label_) {
            lv_obj_set_style_text_color(title_label_, colors.text, 0);
            lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Display), 0);
            lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(40));
        }
        if (tagline_label_) {
            lv_obj_set_style_text_color(tagline_label_, colors.text_secondary, 0);
            lv_obj_set_style_text_font(tagline_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
            lv_obj_align(tagline_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(80));
        }
        if (content_) {
            lv_obj_set_size(content_, LV_HOR_RES - Theme::scalePx(60), Theme::fitHeight(250, 200));
            lv_obj_align(content_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(120));
            lv_obj_set_style_bg_color(content_, colors.fg, 0);
            lv_obj_set_style_border_color(content_, colors.border, 0);
            lv_obj_set_style_text_font(content_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (tip_label_) {
            lv_obj_set_style_text_color(tip_label_, colors.text, 0);
            lv_obj_set_style_text_font(tip_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_set_width(tip_label_, LV_HOR_RES - Theme::scalePx(80));
        }
        if (dismiss_label_) {
            lv_obj_set_style_text_color(dismiss_label_, colors.text_secondary, 0);
            lv_obj_set_style_text_font(dismiss_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
            lv_obj_align(dismiss_label_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(30));
        }
        lv_screen_load(screen_);
    }
}

void ScreenFirstRun::hide() {
    if (dismiss_cb_) {
        dismiss_cb_();
    }
}
