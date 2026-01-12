#include "screen_magic_bar.h"
#include "../scribe_services/ai_assist.h"
#include "../scribe_services/wifi_manager.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_MAGIC_BAR";

ScreenMagicBar::ScreenMagicBar() : bar_(nullptr) {
}

ScreenMagicBar::~ScreenMagicBar() {
    if (bar_) {
        lv_obj_del(bar_);
    }
}

void ScreenMagicBar::init() {
    ESP_LOGI(TAG, "Initializing Magic Bar");

    createWidgets();
}

void ScreenMagicBar::createWidgets() {
    // Create floating bar positioned at bottom
    bar_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(bar_, LV_HOR_RES - 40, 180);
    lv_obj_align(bar_, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(bar_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar_, lv_color_white(), 0);
    lv_obj_set_style_border_width(bar_, 2, 0);
    lv_obj_set_style_border_color(bar_, lv_color_black(), 0);
    lv_obj_set_style_radius(bar_, 10, 0);

    // Style selector at top
    style_selector_ = lv_list_create(bar_);
    lv_obj_set_size(style_selector_, 300, 50);
    lv_obj_align(style_selector_, LV_ALIGN_TOP_MID, 0, 10);

    const AIStyle styles[] = {
        AIStyle::CONTINUE,
        AIStyle::REWRITE,
        AIStyle::SUMMARIZE,
        AIStyle::EXPAND
    };

    for (size_t i = 0; i < style_button_ctx_.size(); i++) {
        AIStyle s = styles[i];
        lv_obj_t* btn = lv_list_add_btn(style_selector_, nullptr, getStyleName(s));
        style_button_ctx_[i] = {this, s};

        lv_obj_add_event_cb(btn, [](lv_obj_t* obj, lv_event_t* event) {
            auto* ctx = static_cast<ScreenMagicBar::StyleButtonContext*>(lv_obj_get_user_data(obj));
            if (!ctx || !ctx->magic) {
                return;
            }
            ctx->magic->style_ = ctx->style;
            ctx->magic->updateStyleSelector();
            if (ctx->magic->style_cb_) {
                ctx->magic->style_cb_(ctx->style);
            }
        }, LV_EVENT_CLICKED, nullptr);

        lv_obj_set_user_data(btn, &style_button_ctx_[i]);
    }

    // Status label
    status_label_ = lv_label_create(bar_);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(status_label_, LV_HOR_RES - 80);
    lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    updateStatus("Press Enter to generate suggestion");

    // Preview text area
    preview_text_ = lv_textarea_create(bar_);
    lv_obj_set_size(preview_text_, LV_HOR_RES - 80, 60);
    lv_obj_align(preview_text_, LV_ALIGN_TOP_MID, 0, 95);
    lv_textarea_set_placeholder_text(preview_text_, "AI suggestion will appear here...");
    lv_textarea_set_text(preview_text_, "");
    lv_obj_add_flag(preview_text_, LV_OBJ_FLAG_CLICKABLE);

    // Action buttons
    lv_obj_t* btn_cont = lv_obj_create(bar_);
    lv_obj_set_size(btn_cont, 200, 40);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);

    // Insert button
    insert_btn_ = lv_btn_create(btn_cont);
    lv_obj_set_size(insert_btn_, 90, 35);
    lv_obj_align(insert_btn_, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_flag(insert_btn_, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* label = lv_label_create(insert_btn_);
    lv_label_set_text(label, "Insert");
    lv_obj_center(label);

    lv_obj_add_event_cb(insert_btn_, [](lv_obj_t* obj, lv_event_t* event) {
        ScreenMagicBar* magic = (ScreenMagicBar*)lv_obj_get_user_data(obj);
        if (magic && magic->insert_cb_) {
            magic->insert_cb_(magic->suggestion_);
            magic->hide();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(insert_btn_, this);

    // Discard button
    discard_btn_ = lv_btn_create(btn_cont);
    lv_obj_set_size(discard_btn_, 90, 35);
    lv_obj_align(discard_btn_, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_flag(discard_btn_, LV_OBJ_FLAG_HIDDEN);

    label = lv_label_create(discard_btn_);
    lv_label_set_text(label, "Discard");
    lv_obj_center(label);

    lv_obj_add_event_cb(discard_btn_, [](lv_obj_t* obj, lv_event_t* event) {
        ScreenMagicBar* magic = (ScreenMagicBar*)lv_obj_get_user_data(obj);
        if (magic) {
            magic->suggestion_.clear();
            lv_textarea_set_text(magic->preview_text_, "");
            lv_obj_add_flag(magic->insert_btn_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(magic->discard_btn_, LV_OBJ_FLAG_HIDDEN);
            magic->state_ = AISuggestionState::IDLE;
            magic->updateStatus("Press Enter to generate suggestion");
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(discard_btn_, this);
}

void ScreenMagicBar::show() {
    if (bar_) {
        lv_obj_clear_flag(bar_, LV_OBJ_FLAG_HIDDEN);
        visible_ = true;

        // Reset state
        suggestion_.clear();
        lv_textarea_set_text(preview_text_, "");
        lv_obj_add_flag(insert_btn_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(discard_btn_, LV_OBJ_FLAG_HIDDEN);
        state_ = AISuggestionState::IDLE;

        updateStyleSelector();

        if (!WiFiManager::getInstance().isConnected()) {
            updateStatus("Offline - Connect to WiFi for AI");
        } else {
            updateStatus("Press Enter to generate suggestion");
        }
    }
}

void ScreenMagicBar::hide() {
    if (bar_) {
        lv_obj_add_flag(bar_, LV_OBJ_FLAG_HIDDEN);
        visible_ = false;
        suggestion_.clear();

        if (cancel_cb_) {
            cancel_cb_();
        }
    }
}

void ScreenMagicBar::onStreamDelta(const char* delta) {
    suggestion_ += delta;
    updatePreview();
}

void ScreenMagicBar::onComplete(bool success, const std::string& error) {
    if (success) {
        state_ = AISuggestionState::READY;
        lv_obj_clear_flag(insert_btn_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(discard_btn_, LV_OBJ_FLAG_HIDDEN);
        updateStatus("Suggestion ready - Insert or Discard");
    } else {
        state_ = AISuggestionState::ERROR;
        updateStatus(error.c_str());
    }
}

void ScreenMagicBar::updateStyleSelector() {
    if (!style_selector_) return;

    // Update selection indicator
    for (int i = 0; i < 4; i++) {
        lv_obj_t* btn = lv_obj_get_child(style_selector_, i);
        if (btn) {
            // Could add visual selection indicator here
        }
    }
}

void ScreenMagicBar::updateStatus(const char* status) {
    if (status_label_) {
        lv_label_set_text(status_label_, status);
    }
}

void ScreenMagicBar::updatePreview() {
    if (preview_text_) {
        lv_textarea_set_text(preview_text_, suggestion_.c_str());
    }
}

const char* ScreenMagicBar::getStyleName(AIStyle style) {
    switch (style) {
        case AIStyle::CONTINUE: return "Continue";
        case AIStyle::REWRITE: return "Rewrite";
        case AIStyle::SUMMARIZE: return "Summarize";
        case AIStyle::EXPAND: return "Expand";
        case AIStyle::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}
