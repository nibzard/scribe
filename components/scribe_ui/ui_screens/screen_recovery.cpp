#include "screen_recovery.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_RECOVERY";

ScreenRecovery::ScreenRecovery() : screen_(nullptr) {
}

ScreenRecovery::~ScreenRecovery() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenRecovery::init() {
    ESP_LOGI(TAG, "Initializing recovery screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenRecovery::createWidgets() {
    // Warning icon
    icon_label_ = lv_label_create(screen_);
    lv_label_set_text(icon_label_, LV_SYMBOL_WARNING);
    lv_obj_align(icon_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(40));
    lv_obj_set_style_text_font(icon_label_, Theme::getUIFont(Theme::UiFontRole::Display), 0);
    lv_obj_set_style_text_color(icon_label_, Theme::getColors().warning, 0);

    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("storage.recovery_title"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(90));
    lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);

    // Message
    message_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(message_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(message_label_, Theme::scalePx(300));
    lv_label_set_text(message_label_, Strings::getInstance().get("storage.recovery_body"));
    lv_obj_align(message_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(120));
    lv_obj_set_style_text_align(message_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(message_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    preview_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(preview_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(preview_label_, Theme::scalePx(320));
    lv_label_set_text(preview_label_, "");
    lv_obj_align(preview_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(180));
    lv_obj_set_style_text_font(preview_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Buttons
    restore_btn_ = lv_button_create(screen_);
    lv_obj_set_size(restore_btn_, Theme::scalePx(140), Theme::scalePx(50));
    lv_obj_align(restore_btn_, LV_ALIGN_CENTER, -Theme::scalePx(80), Theme::scalePx(60));
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(restore_btn_, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(restore_btn_, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(restore_btn_, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(restore_btn_, colors.text, LV_PART_MAIN);
    lv_obj_set_style_text_font(restore_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    lv_obj_t* label = lv_label_create(restore_btn_);
    lv_label_set_text(label, Strings::getInstance().get("storage.recovery_restore"));
    lv_obj_center(label);
    lv_obj_set_user_data(restore_btn_, this);
    lv_obj_add_event_cb(restore_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenRecovery* screen = static_cast<ScreenRecovery*>(lv_obj_get_user_data(target));
        if (!screen) {
            return;
        }
        screen->setSelectedRestore(true);
        if (screen->recovery_cb_) {
            screen->recovery_cb_(true);
        }
    }, LV_EVENT_CLICKED, nullptr);

    keep_btn_ = lv_button_create(screen_);
    lv_obj_set_size(keep_btn_, Theme::scalePx(140), Theme::scalePx(50));
    lv_obj_align(keep_btn_, LV_ALIGN_CENTER, Theme::scalePx(80), Theme::scalePx(60));
    lv_obj_set_style_bg_color(keep_btn_, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(keep_btn_, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(keep_btn_, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(keep_btn_, colors.text, LV_PART_MAIN);
    lv_obj_set_style_text_font(keep_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    label = lv_label_create(keep_btn_);
    lv_label_set_text(label, Strings::getInstance().get("storage.recovery_keep"));
    lv_obj_center(label);
    lv_obj_set_user_data(keep_btn_, this);
    lv_obj_add_event_cb(keep_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenRecovery* screen = static_cast<ScreenRecovery*>(lv_obj_get_user_data(target));
        if (!screen) {
            return;
        }
        screen->setSelectedRestore(false);
        if (screen->recovery_cb_) {
            screen->recovery_cb_(false);
        }
    }, LV_EVENT_CLICKED, nullptr);

}

void ScreenRecovery::show(const std::string& recovered_text) {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        const Theme::Colors& colors = Theme::getColors();
        lv_obj_set_style_text_font(screen_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        if (icon_label_) {
            lv_obj_set_style_text_color(icon_label_, colors.warning, 0);
            lv_obj_set_style_text_font(icon_label_, Theme::getUIFont(Theme::UiFontRole::Display), 0);
            lv_obj_align(icon_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(40));
        }
        if (title_label_) {
            lv_obj_set_style_text_color(title_label_, colors.text, 0);
            lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
            lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(90));
        }
        if (message_label_) {
            lv_obj_set_style_text_color(message_label_, colors.text, 0);
            lv_obj_set_style_text_font(message_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_set_width(message_label_, Theme::scalePx(300));
            lv_obj_align(message_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(120));
        }
        if (preview_label_) {
            lv_obj_set_style_text_color(preview_label_, colors.text_secondary, 0);
            lv_obj_set_style_text_font(preview_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_set_width(preview_label_, Theme::scalePx(320));
            lv_obj_align(preview_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(180));
        }
        if (restore_btn_) {
            lv_obj_set_size(restore_btn_, Theme::scalePx(140), Theme::scalePx(50));
            lv_obj_align(restore_btn_, LV_ALIGN_CENTER, -Theme::scalePx(80), Theme::scalePx(60));
            lv_obj_set_style_bg_color(restore_btn_, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(restore_btn_, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(restore_btn_, colors.text, LV_PART_MAIN);
            lv_obj_set_style_text_font(restore_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (keep_btn_) {
            lv_obj_set_size(keep_btn_, Theme::scalePx(140), Theme::scalePx(50));
            lv_obj_align(keep_btn_, LV_ALIGN_CENTER, Theme::scalePx(80), Theme::scalePx(60));
            lv_obj_set_style_bg_color(keep_btn_, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(keep_btn_, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(keep_btn_, colors.text, LV_PART_MAIN);
            lv_obj_set_style_text_font(keep_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        lv_screen_load(screen_);
    }

    std::string preview = recovered_text.substr(0, 200);
    if (recovered_text.length() > preview.length()) {
        preview += "...";
    }
    if (preview_label_) {
        lv_label_set_text(preview_label_, preview.c_str());
    }
    ESP_LOGI(TAG, "Recovered %zu characters", recovered_text.length());
}

void ScreenRecovery::setSelectedRestore(bool restore) {
    if (restore_btn_) {
        if (restore) {
            lv_obj_add_state(restore_btn_, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(restore_btn_, LV_STATE_CHECKED);
        }
    }
    if (keep_btn_) {
        if (!restore) {
            lv_obj_add_state(keep_btn_, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(keep_btn_, LV_STATE_CHECKED);
        }
    }
}

void ScreenRecovery::hide() {
    // Screen will be hidden when another is loaded
}
