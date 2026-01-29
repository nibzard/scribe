#include "screen_ai.h"
#include "../../scribe_utils/strings.h"
#include "../scribe_services/ai_assist.h"
#include "../scribe_secrets/secrets_nvs.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_AI";

ScreenAI::ScreenAI() : screen_(nullptr) {
}

ScreenAI::~ScreenAI() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenAI::init() {
    ESP_LOGI(TAG, "Initializing AI settings screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenAI::createWidgets() {
    const Theme::Colors& colors = Theme::getColors();
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("ai.title"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(30));
    lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
    lv_obj_set_style_text_color(title_label_, colors.text, 0);

    // Description
    description_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(description_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(description_label_, Theme::fitWidth(350, 40));
    lv_label_set_text(description_label_, Strings::getInstance().get("ai.off_desc"));
    lv_obj_align(description_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(70));
    lv_obj_set_style_text_align(description_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(description_label_, colors.text, 0);
    lv_obj_set_style_text_font(description_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // API key label
    key_prompt_label_ = lv_label_create(screen_);
    lv_label_set_text(key_prompt_label_, Strings::getInstance().get("ai.key_prompt"));
    lv_obj_align(key_prompt_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(160));
    lv_obj_set_style_text_font(key_prompt_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // API key input
    api_key_input_ = lv_textarea_create(screen_);
    lv_obj_set_size(api_key_input_, Theme::fitWidth(350, 40), Theme::scalePx(50));
    lv_obj_align(api_key_input_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(190));
    lv_textarea_set_placeholder_text(api_key_input_, Strings::getInstance().get("ai.key_placeholder"));
    lv_textarea_set_password_mode(api_key_input_, true);
    lv_textarea_set_one_line(api_key_input_, true);
    lv_obj_add_flag(api_key_input_, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_set_style_bg_color(api_key_input_, colors.fg, 0);
    lv_obj_set_style_bg_opa(api_key_input_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(api_key_input_, colors.border, 0);
    lv_obj_set_style_border_width(api_key_input_, 1, 0);
    lv_obj_set_style_text_color(api_key_input_, colors.text, 0);
    lv_obj_set_style_text_color(api_key_input_, colors.text_secondary, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_font(api_key_input_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Key hint
    hint_label_ = lv_label_create(screen_);
    lv_label_set_text(hint_label_, Strings::getInstance().get("ai.key_hint"));
    lv_obj_align(hint_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(250));
    lv_obj_set_style_text_align(hint_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(hint_label_, colors.text_secondary, 0);
    lv_obj_set_style_text_font(hint_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);

    // Status label
    status_label_ = lv_label_create(screen_);
    lv_obj_set_width(status_label_, Theme::fitWidth(350, 40));
    lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(275));
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label_, colors.text, 0);
    lv_obj_set_style_text_font(status_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Save button
    save_btn_ = lv_button_create(screen_);
    lv_obj_set_size(save_btn_, Theme::scalePx(120), Theme::scalePx(50));
    lv_obj_align(save_btn_, LV_ALIGN_BOTTOM_MID, -Theme::scalePx(70), -Theme::scalePx(20));
    lv_obj_set_style_text_font(save_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    lv_obj_t* btn_label = lv_label_create(save_btn_);
    lv_label_set_text(btn_label, Strings::getInstance().get("common.save"));
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(save_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenAI* screen = (ScreenAI*)lv_obj_get_user_data(target);
        if (screen) {
            screen->saveAPIKey();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(save_btn_, this);

    // Back button
    back_btn_ = lv_button_create(screen_);
    lv_obj_set_size(back_btn_, Theme::scalePx(120), Theme::scalePx(50));
    lv_obj_align(back_btn_, LV_ALIGN_BOTTOM_MID, Theme::scalePx(70), -Theme::scalePx(20));
    lv_obj_set_style_text_font(back_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    btn_label = lv_label_create(back_btn_);
    lv_label_set_text(btn_label, Strings::getInstance().get("common.back"));
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(back_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenAI* screen = (ScreenAI*)lv_obj_get_user_data(target);
        if (screen && screen->back_cb_) {
            screen->back_cb_();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(back_btn_, this);
}

void ScreenAI::show() {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        const Theme::Colors& colors = Theme::getColors();
        lv_obj_set_style_text_font(screen_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        if (title_label_) {
            lv_obj_set_style_text_color(title_label_, colors.text, 0);
            lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
            lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(30));
        }
        if (description_label_) {
            lv_obj_set_style_text_color(description_label_, colors.text, 0);
            lv_obj_set_style_text_font(description_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_set_width(description_label_, Theme::fitWidth(350, 40));
            lv_obj_align(description_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(70));
        }
        if (key_prompt_label_) {
            lv_obj_set_style_text_color(key_prompt_label_, colors.text, 0);
            lv_obj_set_style_text_font(key_prompt_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_align(key_prompt_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(160));
        }
        if (hint_label_) {
            lv_obj_set_style_text_color(hint_label_, colors.text_secondary, 0);
            lv_obj_set_style_text_font(hint_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
            lv_obj_align(hint_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(250));
        }
        if (status_label_) {
            lv_obj_set_style_text_color(status_label_, colors.text, 0);
            lv_obj_set_style_text_font(status_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_set_width(status_label_, Theme::fitWidth(350, 40));
            lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(275));
        }
        if (api_key_input_) {
            lv_obj_set_size(api_key_input_, Theme::fitWidth(350, 40), Theme::scalePx(50));
            lv_obj_align(api_key_input_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(190));
            lv_obj_set_style_bg_color(api_key_input_, colors.fg, 0);
            lv_obj_set_style_border_color(api_key_input_, colors.border, 0);
            lv_obj_set_style_text_color(api_key_input_, colors.text, 0);
            lv_obj_set_style_text_color(api_key_input_, colors.text_secondary, LV_PART_TEXTAREA_PLACEHOLDER);
            lv_obj_set_style_text_font(api_key_input_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (save_btn_) {
            lv_obj_set_size(save_btn_, Theme::scalePx(120), Theme::scalePx(50));
            lv_obj_align(save_btn_, LV_ALIGN_BOTTOM_MID, -Theme::scalePx(70), -Theme::scalePx(20));
            lv_obj_set_style_text_font(save_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (back_btn_) {
            lv_obj_set_size(back_btn_, Theme::scalePx(120), Theme::scalePx(50));
            lv_obj_align(back_btn_, LV_ALIGN_BOTTOM_MID, Theme::scalePx(70), -Theme::scalePx(20));
            lv_obj_set_style_text_font(back_btn_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        lv_screen_load(screen_);

        // Load existing API key
        std::string key = AIAssist::getInstance().getAPIKey();
        if (!key.empty()) {
            lv_textarea_set_text(api_key_input_, key.c_str());
        }

        updateStatus();
    }
}

void ScreenAI::hide() {
    if (back_cb_) {
        back_cb_();
    }
}

void ScreenAI::updateStatus() {
    if (!status_label_) return;

    AIAssist& ai = AIAssist::getInstance();
    std::string key = ai.getAPIKey();

    if (!key.empty()) {
        lv_label_set_text(status_label_, ai.isEnabled()
            ? Strings::getInstance().get("ai.status_enabled")
            : Strings::getInstance().get("ai.status_disabled"));
    } else {
        lv_label_set_text(status_label_, Strings::getInstance().get("ai.status_disabled"));
    }
}

void ScreenAI::saveAPIKey() {
    if (!api_key_input_) return;

    const char* key = lv_textarea_get_text(api_key_input_);

    // Basic validation
    size_t len = strlen(key);
    if (len < 20) {
        lv_label_set_text(status_label_, Strings::getInstance().get("ai.key_invalid"));
        return;
    }

    if (len > 100) {
        lv_label_set_text(status_label_, Strings::getInstance().get("ai.key_invalid"));
        return;
    }

    // Save key
    esp_err_t ret = AIAssist::getInstance().setAPIKey(std::string(key));

    if (ret == ESP_OK) {
        lv_label_set_text(status_label_, Strings::getInstance().get("ai.key_saved"));
        AIAssist::getInstance().setEnabled(true);
    } else {
        lv_label_set_text(status_label_, Strings::getInstance().get("ai.error"));
    }
}
