#include "screen_magic_bar.h"
#include "../../scribe_utils/strings.h"
#include "../scribe_services/wifi_manager.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_MAGIC_BAR";

ScreenMagicBar::ScreenMagicBar() : bar_(nullptr) {
}

ScreenMagicBar::~ScreenMagicBar() {
    if (bar_) {
        lv_obj_delete(bar_);
    }
}

void ScreenMagicBar::init() {
    ESP_LOGI(TAG, "Initializing Magic Bar");

    createWidgets();
}

void ScreenMagicBar::createWidgets() {
    const Theme::Colors& colors = Theme::getColors();
    // Create floating bar positioned at bottom
    bar_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(bar_, LV_HOR_RES - Theme::scalePx(40), Theme::scalePx(180));
    lv_obj_align(bar_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(10));
    lv_obj_set_style_bg_opa(bar_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar_, colors.fg, 0);
    lv_obj_set_style_border_width(bar_, 2, 0);
    lv_obj_set_style_border_color(bar_, colors.border, 0);
    lv_obj_set_style_radius(bar_, Theme::scalePx(10), 0);
    lv_obj_set_style_text_font(bar_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
    lv_obj_add_flag(bar_, LV_OBJ_FLAG_HIDDEN);

    // Status label
    status_label_ = lv_label_create(bar_);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(status_label_, LV_HOR_RES - Theme::scalePx(80));
    lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(12));
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label_, colors.text_secondary, 0);
    lv_obj_set_style_text_font(status_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
    updateStatus(Strings::getInstance().get("ai.status_idle"));

    // Preview box
    preview_box_ = lv_obj_create(bar_);
    lv_obj_set_size(preview_box_, LV_HOR_RES - Theme::scalePx(80), Theme::scalePx(80));
    lv_obj_align(preview_box_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(42));
    lv_obj_set_style_bg_color(preview_box_, colors.bg, 0);
    lv_obj_set_style_bg_opa(preview_box_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(preview_box_, colors.border, 0);
    lv_obj_set_style_border_width(preview_box_, 1, 0);
    lv_obj_set_style_radius(preview_box_, Theme::scalePx(8), 0);
    lv_obj_set_style_pad_all(preview_box_, Theme::scalePx(8), 0);
    lv_obj_set_style_text_font(preview_box_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    preview_label_ = lv_label_create(preview_box_);
    lv_label_set_long_mode(preview_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(preview_label_, LV_HOR_RES - Theme::scalePx(100));
    lv_obj_align(preview_label_, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_color(preview_label_, colors.text, 0);
    lv_obj_set_style_text_font(preview_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
    lv_label_set_text(preview_label_, "");

    // Action buttons
    btn_cont_ = lv_obj_create(bar_);
    lv_obj_set_size(btn_cont_, Theme::scalePx(200), Theme::scalePx(40));
    lv_obj_align(btn_cont_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(10));
    lv_obj_set_style_bg_opa(btn_cont_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_cont_, 0, 0);
    lv_obj_set_style_text_font(btn_cont_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Insert button
    insert_btn_ = lv_button_create(btn_cont_);
    lv_obj_set_size(insert_btn_, Theme::scalePx(90), Theme::scalePx(35));
    lv_obj_align(insert_btn_, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_flag(insert_btn_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_font(insert_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    lv_obj_t* label = lv_label_create(insert_btn_);
    lv_label_set_text(label, Strings::getInstance().get("ai.accept"));
    lv_obj_center(label);

    lv_obj_add_event_cb(insert_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenMagicBar* magic = (ScreenMagicBar*)lv_obj_get_user_data(target);
        if (magic) {
            magic->acceptSuggestion();
            magic->hide();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(insert_btn_, this);

    // Discard button
    discard_btn_ = lv_button_create(btn_cont_);
    lv_obj_set_size(discard_btn_, Theme::scalePx(90), Theme::scalePx(35));
    lv_obj_align(discard_btn_, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_flag(discard_btn_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_font(discard_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    label = lv_label_create(discard_btn_);
    lv_label_set_text(label, Strings::getInstance().get("ai.reject"));
    lv_obj_center(label);

    lv_obj_add_event_cb(discard_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenMagicBar* magic = (ScreenMagicBar*)lv_obj_get_user_data(target);
        if (magic) {
            magic->discardSuggestion();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(discard_btn_, this);
}

void ScreenMagicBar::show() {
    if (bar_) {
        const Theme::Colors& colors = Theme::getColors();
        lv_obj_set_size(bar_, LV_HOR_RES - Theme::scalePx(40), Theme::scalePx(180));
        lv_obj_align(bar_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(10));
        lv_obj_set_style_bg_color(bar_, colors.fg, 0);
        lv_obj_set_style_border_color(bar_, colors.border, 0);
        lv_obj_set_style_text_font(bar_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        if (preview_box_) {
            lv_obj_set_size(preview_box_, LV_HOR_RES - Theme::scalePx(80), Theme::scalePx(80));
            lv_obj_align(preview_box_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(42));
            lv_obj_set_style_bg_color(preview_box_, colors.bg, 0);
            lv_obj_set_style_border_color(preview_box_, colors.border, 0);
            lv_obj_set_style_text_font(preview_box_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (preview_label_) {
            lv_obj_set_style_text_color(preview_label_, colors.text, 0);
            lv_obj_set_style_text_font(preview_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_set_width(preview_label_, LV_HOR_RES - Theme::scalePx(100));
        }
        if (status_label_) {
            lv_obj_set_style_text_color(status_label_, colors.text_secondary, 0);
            lv_obj_set_style_text_font(status_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
            lv_obj_set_width(status_label_, LV_HOR_RES - Theme::scalePx(80));
            lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(12));
        }
        if (btn_cont_) {
            lv_obj_set_size(btn_cont_, Theme::scalePx(200), Theme::scalePx(40));
            lv_obj_align(btn_cont_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(10));
            lv_obj_set_style_text_font(btn_cont_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (insert_btn_) {
            lv_obj_set_size(insert_btn_, Theme::scalePx(90), Theme::scalePx(35));
            lv_obj_set_style_text_font(insert_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (discard_btn_) {
            lv_obj_set_size(discard_btn_, Theme::scalePx(90), Theme::scalePx(35));
            lv_obj_set_style_text_font(discard_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        lv_obj_clear_flag(bar_, LV_OBJ_FLAG_HIDDEN);
        visible_ = true;

        // Reset state
        suggestion_.clear();
        if (preview_label_) {
            lv_label_set_text(preview_label_, "");
        }
        lv_obj_add_flag(insert_btn_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(discard_btn_, LV_OBJ_FLAG_HIDDEN);
        state_ = AISuggestionState::IDLE;

        if (!WiFiManager::getInstance().isConnected()) {
            updateStatus(Strings::getInstance().get("ai.offline"));
        } else {
            updateStatus(Strings::getInstance().get("ai.status_idle"));
        }
    }
}

void ScreenMagicBar::hide() {
    if (bar_) {
        lv_obj_add_flag(bar_, LV_OBJ_FLAG_HIDDEN);
        visible_ = false;
        suggestion_.clear();
        state_ = AISuggestionState::IDLE;
    }
}

void ScreenMagicBar::onStreamDelta(const char* delta) {
    if (state_ != AISuggestionState::GENERATING) {
        state_ = AISuggestionState::GENERATING;
        updateStatus(Strings::getInstance().get("ai.generating"));
    }
    suggestion_ += delta;
    updatePreview();
}

void ScreenMagicBar::onComplete(bool success, const std::string& error) {
    if (success) {
        state_ = AISuggestionState::READY;
        lv_obj_clear_flag(insert_btn_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(discard_btn_, LV_OBJ_FLAG_HIDDEN);
        updateStatus(Strings::getInstance().get("ai.status_ready"));
    } else {
        state_ = AISuggestionState::ERROR;
        updateStatus(error.c_str());
    }
}

void ScreenMagicBar::updateStatus(const char* status) {
    if (status_label_) {
        lv_label_set_text(status_label_, status);
    }
}

void ScreenMagicBar::updatePreview() {
    if (preview_label_) {
        lv_label_set_text(preview_label_, suggestion_.c_str());
    }
}

void ScreenMagicBar::startGenerating() {
    suggestion_.clear();
    if (preview_label_) {
        lv_label_set_text(preview_label_, "");
    }
    if (insert_btn_) {
        lv_obj_add_flag(insert_btn_, LV_OBJ_FLAG_HIDDEN);
    }
    if (discard_btn_) {
        lv_obj_add_flag(discard_btn_, LV_OBJ_FLAG_HIDDEN);
    }
    state_ = AISuggestionState::GENERATING;
    updateStatus(Strings::getInstance().get("ai.generating"));
}

void ScreenMagicBar::acceptSuggestion() {
    if (state_ != AISuggestionState::READY) {
        return;
    }
    if (insert_cb_) {
        insert_cb_(suggestion_);
    }
}

void ScreenMagicBar::discardSuggestion() {
    if (cancel_cb_) {
        cancel_cb_();
        return;
    }

    suggestion_.clear();
    if (preview_label_) {
        lv_label_set_text(preview_label_, "");
    }
    if (insert_btn_) {
        lv_obj_add_flag(insert_btn_, LV_OBJ_FLAG_HIDDEN);
    }
    if (discard_btn_) {
        lv_obj_add_flag(discard_btn_, LV_OBJ_FLAG_HIDDEN);
    }
    state_ = AISuggestionState::IDLE;
    updateStatus(Strings::getInstance().get("ai.status_idle"));
}
