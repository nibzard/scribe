#include "screen_ai.h"
#include "../scribe_services/ai_assist.h"
#include "../scribe_secrets/secrets_nvs.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_AI";

ScreenAI::ScreenAI() : screen_(nullptr) {
}

ScreenAI::~ScreenAI() {
    if (screen_) {
        lv_obj_del(screen_);
    }
}

void ScreenAI::init() {
    ESP_LOGI(TAG, "Initializing AI settings screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenAI::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, "AI Assistant");
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_20, 0);

    // Description
    description_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(description_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(description_label_, 350);
    lv_label_set_text(description_label_,
        "AI writing assistance powered by OpenAI.\n\n"
        "Your writing is processed securely. Requires WiFi connection.\n\n"
        "To enable AI, add your OpenAI API key below.");
    lv_obj_align(description_label_, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_text_align(description_label_, LV_TEXT_ALIGN_CENTER, 0);

    // API key label
    lv_obj_t* label = lv_label_create(screen_);
    lv_label_set_text(label, "OpenAI API Key (sk-...):");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 160);

    // API key input
    api_key_input_ = lv_textarea_create(screen_);
    lv_obj_set_size(api_key_input_, 350, 50);
    lv_obj_align(api_key_input_, LV_ALIGN_TOP_MID, 0, 190);
    lv_textarea_set_placeholder_text(api_key_input_, "sk-...");
    lv_textarea_set_password_mode(api_key_input_, true);
    lv_textarea_set_one_line(api_key_input_, true);
    lv_obj_add_flag(api_key_input_, LV_OBJ_FLAG_FOCUSABLE);

    // Status label
    status_label_ = lv_label_create(screen_);
    lv_obj_set_width(status_label_, 350);
    lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, 260);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);

    // Save button
    save_btn_ = lv_btn_create(screen_);
    lv_obj_set_size(save_btn_, 120, 50);
    lv_obj_align(save_btn_, LV_ALIGN_BOTTOM_MID, -70, -20);

    lv_obj_t* btn_label = lv_label_create(save_btn_);
    lv_label_set_text(btn_label, "Save");
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(save_btn_, [](lv_obj_t* obj, lv_event_t* event) {
        ScreenAI* screen = (ScreenAI*)lv_obj_get_user_data(obj);
        if (screen) {
            screen->saveAPIKey();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(save_btn_, this);

    // Back button
    back_btn_ = lv_btn_create(screen_);
    lv_obj_set_size(back_btn_, 120, 50);
    lv_obj_align(back_btn_, LV_ALIGN_BOTTOM_MID, 70, -20);

    btn_label = lv_label_create(back_btn_);
    lv_label_set_text(btn_label, "Back");
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(back_btn_, [](lv_obj_t* obj, lv_event_t* event) {
        ScreenAI* screen = (ScreenAI*)lv_obj_get_user_data(obj);
        if (screen && screen->back_cb_) {
            screen->back_cb_();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(back_btn_, this);
}

void ScreenAI::show() {
    if (screen_) {
        lv_scr_load(screen_);

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
        if (ai.isEnabled()) {
            lv_label_set_text(status_label_, "API key configured - AI enabled");
        } else {
            lv_label_set_text(status_label_, "API key configured - AI disabled in settings");
        }
    } else {
        lv_label_set_text(status_label_, "No API key - AI features unavailable");
    }
}

void ScreenAI::saveAPIKey() {
    if (!api_key_input_) return;

    const char* key = lv_textarea_get_text(api_key_input_);

    // Basic validation
    size_t len = strlen(key);
    if (len < 20) {
        lv_label_set_text(status_label_, "Error: API key too short (min 20 chars)");
        return;
    }

    if (len > 100) {
        lv_label_set_text(status_label_, "Error: API key too long");
        return;
    }

    // Save key
    esp_err_t ret = AIAssist::getInstance().setAPIKey(std::string(key));

    if (ret == ESP_OK) {
        lv_label_set_text(status_label_, "API key saved successfully!");
        AIAssist::getInstance().setEnabled(true);
    } else {
        lv_label_set_text(status_label_, "Error saving API key");
    }
}
