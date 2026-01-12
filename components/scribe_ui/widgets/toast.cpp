#include "toast.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_TOAST";

// Toast colors
#define TOAST_BG_COLOR    lv_color_hex(0x333333)
#define TOAST_TEXT_COLOR  lv_color_white()
#define TOAST_SUCCESS_COLOR lv_color_hex(0x4CAF50)
#define TOAST_ERROR_COLOR   lv_color_hex(0xF44336)
#define TOAST_WARN_COLOR    lv_color_hex(0xFFC107)

Toast::Toast(lv_obj_t* parent) {
    // Create toast container on top layer
    container_ = lv_obj_create(lv_layer_top());
    if (!container_) {
        ESP_LOGE(TAG, "Failed to create toast container");
        return;
    }

    // Configure container
    lv_obj_set_size(container_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(container_, LV_PCT(50), LV_PCT(85));  // Bottom center
    lv_obj_set_style_bg_color(container_, TOAST_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_90, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_radius(container_, 8, 0);
    lv_obj_set_style_pad_all(container_, 12, 0);
    lv_obj_set_style_pad_hor(container_, 20, 0);
    lv_obj_set_style_pad_ver(container_, 10, 0);

    // Initially hidden
    lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
    visible_ = false;

    // Create label
    label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(label_, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_, TOAST_TEXT_COLOR, 0);
    lv_label_set_long_mode(label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label_, LV_PCT(80));  // Max width 80% of screen
    lv_label_set_text(label_, "");

    ESP_LOGI(TAG, "Toast widget created");
}

Toast::~Toast() {
    if (hide_timer_) {
        lv_timer_del(hide_timer_);
        hide_timer_ = nullptr;
    }
    if (container_) {
        lv_obj_delete(container_);
        container_ = nullptr;
    }
}

void Toast::show(const char* message, uint32_t duration_ms) {
    if (!container_ || !label_) {
        return;
    }

    lv_label_set_text(label_, message);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(container_);
    visible_ = true;

    // Center the toast
    lv_obj_center(container_);

    ESP_LOGD(TAG, "Toast shown: %s", message);

    startHideTimer(duration_ms);
}

void Toast::show(const std::string& message, uint32_t duration_ms) {
    show(message.c_str(), duration_ms);
}

void Toast::hide() {
    if (container_ && visible_) {
        lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
        visible_ = false;
        ESP_LOGD(TAG, "Toast hidden");

        if (hide_callback_) {
            hide_callback_();
        }
    }
}

void Toast::startHideTimer(uint32_t duration_ms) {
    if (hide_timer_) {
        lv_timer_del(hide_timer_);
        hide_timer_ = nullptr;
    }

    if (duration_ms > 0) {
        hide_timer_ = lv_timer_create([](lv_timer_t* t) {
            Toast* toast = static_cast<Toast*>(lv_timer_get_user_data(t));
            if (toast) {
                toast->hide();
            }
        }, duration_ms, this);
    }
}
