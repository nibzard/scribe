#include "screen_recovery.h"
#include "../../scribe_utils/strings.h"
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
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenRecovery::createWidgets() {
    // Warning icon
    lv_obj_t* icon = lv_label_create(screen_);
    lv_label_set_text(icon, LV_SYMBOL_WARNING);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xFF9800), 0);

    // Title
    lv_obj_t* title = lv_label_create(screen_);
    lv_label_set_text(title, Strings::getInstance().get("storage.recovery_title"));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

    // Message
    message_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(message_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(message_label_, 300);
    lv_label_set_text(message_label_, Strings::getInstance().get("storage.recovery_body"));
    lv_obj_align(message_label_, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_style_text_align(message_label_, LV_TEXT_ALIGN_CENTER, 0);

    preview_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(preview_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(preview_label_, 320);
    lv_label_set_text(preview_label_, "");
    lv_obj_align(preview_label_, LV_ALIGN_TOP_MID, 0, 180);

    // Buttons
    restore_btn_ = lv_button_create(screen_);
    lv_obj_set_size(restore_btn_, 140, 50);
    lv_obj_align(restore_btn_, LV_ALIGN_CENTER, -80, 60);

    lv_obj_t* label = lv_label_create(restore_btn_);
    lv_label_set_text(label, Strings::getInstance().get("storage.recovery_restore"));
    lv_obj_center(label);

    keep_btn_ = lv_button_create(screen_);
    lv_obj_set_size(keep_btn_, 140, 50);
    lv_obj_align(keep_btn_, LV_ALIGN_CENTER, 80, 60);

    label = lv_label_create(keep_btn_);
    lv_label_set_text(label, Strings::getInstance().get("storage.recovery_keep"));
    lv_obj_center(label);

}

void ScreenRecovery::show(const std::string& recovered_text) {
    if (screen_) {
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
