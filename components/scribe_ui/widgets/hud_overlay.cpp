#include "hud_overlay.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_HUD";

// HUD colors
#define HUD_BG_COLOR      lv_color_hex(0x1a1a1a)
#define HUD_TEXT_COLOR    lv_color_white()
#define HUD_SUCCESS_COLOR lv_color_hex(0x4CAF50)
#define HUD_WARN_COLOR    lv_color_hex(0xFFC107)
#define HUD_INFO_COLOR    lv_color_hex(0x2196F3)

HUDOverlay::HUDOverlay(lv_obj_t* parent) {
    // Create overlay on top layer
    overlay_ = lv_obj_create(lv_layer_top());
    if (!overlay_) {
        ESP_LOGE(TAG, "Failed to create HUD overlay");
        return;
    }

    // Configure overlay to cover entire screen
    lv_obj_set_size(overlay_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(overlay_, 0, 0);
    lv_obj_set_style_bg_opa(overlay_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(overlay_, 0, 0);
    lv_obj_set_style_pad_all(overlay_, 0, 0);

    // Initially hidden
    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);
    visible_ = false;

    createWidgets();
    ESP_LOGI(TAG, "HUD overlay created");
}

HUDOverlay::~HUDOverlay() {
    if (overlay_) {
        lv_obj_del(overlay_);
        overlay_ = nullptr;
    }
}

void HUDOverlay::createWidgets() {
    // Create main container (semi-transparent dark box)
    container_ = lv_obj_create(overlay_);
    lv_obj_set_size(container_, 280, 180);
    lv_obj_center(container_);
    lv_obj_set_style_bg_color(container_, HUD_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_90, 0);
    lv_obj_set_style_border_width(container_, 2, 0);
    lv_obj_set_style_border_color(container_, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(container_, 10, 0);
    lv_obj_set_style_pad_all(container_, 15, 0);

    // Project name label (title)
    project_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(project_label_, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(project_label_, HUD_TEXT_COLOR, 0);
    lv_label_set_long_mode(project_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_width(project_label_, 250);
    lv_obj_align(project_label_, LV_ALIGN_TOP_MID, 0, 0);

    // Separator line
    lv_obj_t* separator = lv_obj_create(container_);
    lv_obj_set_size(separator, 250, 1);
    lv_obj_set_style_bg_color(separator, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_opa(separator, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(separator, 0, 0);
    lv_obj_align(separator, LV_ALIGN_TOP_MID, 0, 30);

    // Word counts section
    words_today_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(words_today_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(words_today_label_, HUD_TEXT_COLOR, 0);
    lv_obj_align(words_today_label_, LV_ALIGN_TOP_LEFT, 0, 40);
    lv_label_set_text(words_today_label_, "Today: 0");

    words_total_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(words_total_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(words_total_label_, HUD_TEXT_COLOR, 0);
    lv_obj_align(words_total_label_, LV_ALIGN_TOP_LEFT, 140, 40);
    lv_label_set_text(words_total_label_, "Total: 0");

    // Battery label
    battery_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(battery_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(battery_label_, HUD_TEXT_COLOR, 0);
    lv_obj_align(battery_label_, LV_ALIGN_TOP_LEFT, 0, 70);
    lv_label_set_text(battery_label_, "Battery: --%");

    // Save state label
    save_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(save_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(save_label_, HUD_SUCCESS_COLOR, 0);
    lv_obj_align(save_label_, LV_ALIGN_TOP_LEFT, 0, 100);
    lv_label_set_text(save_label_, "Saved \u2713");

    // Backup state label
    backup_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(backup_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(backup_label_, HUD_INFO_COLOR, 0);
    lv_obj_align(backup_label_, LV_ALIGN_TOP_LEFT, 0, 125);
    lv_label_set_text(backup_label_, "Backup: off");

    // AI state label
    ai_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(ai_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ai_label_, lv_color_hex(0x9E9E9E), 0);
    lv_obj_align(ai_label_, LV_ALIGN_TOP_LEFT, 140, 125);
    lv_label_set_text(ai_label_, "AI: off");

    ESP_LOGI(TAG, "HUD widgets created");
}

void HUDOverlay::setProjectName(const std::string& name) {
    if (project_label_) {
        lv_label_set_text_fmt(project_label_, "%s", name.c_str());
    }
}

void HUDOverlay::setWordCounts(size_t today, size_t total) {
    if (words_today_label_) {
        lv_label_set_text_fmt(words_today_label_, "Today: %zu", today);
    }
    if (words_total_label_) {
        lv_label_set_text_fmt(words_total_label_, "Total: %zu", total);
    }
}

void HUDOverlay::setBattery(int percentage, bool charging) {
    if (battery_label_) {
        const char* icon = charging ? "\u26A1" : "";
        lv_label_set_text_fmt(battery_label_, "%s Battery: %d%%", icon, percentage);

        // Color code battery level
        lv_color_t color = HUD_TEXT_COLOR;
        if (percentage <= 20) {
            color = lv_color_hex(0xF44336);  // Red
        } else if (percentage <= 50) {
            color = HUD_WARN_COLOR;  // Yellow/Orange
        }
        lv_obj_set_style_text_color(battery_label_, color, 0);
    }
}

void HUDOverlay::setSaveState(const std::string& state) {
    if (save_label_) {
        lv_label_set_text(save_label_, state.c_str());

        // Color code based on state
        lv_color_t color = HUD_TEXT_COLOR;
        if (state.find("Saved") != std::string::npos ||
            state.find("\u2713") != std::string::npos) {
            color = HUD_SUCCESS_COLOR;
        } else if (state.find("Saving") != std::string::npos ||
                   state.find("\u2026") != std::string::npos) {
            color = HUD_WARN_COLOR;
        }
        lv_obj_set_style_text_color(save_label_, color, 0);
    }
}

void HUDOverlay::setBackupState(const std::string& state) {
    if (backup_label_) {
        lv_label_set_text(backup_label_, state.c_str());

        // Show/hide and color based on state
        if (state.find("off") != std::string::npos) {
            lv_obj_set_style_text_color(backup_label_, lv_color_hex(0x9E9E9E), 0);
        } else if (state.find("synced") != std::string::npos ||
                   state.find("\u2713") != std::string::npos) {
            lv_obj_set_style_text_color(backup_label_, HUD_SUCCESS_COLOR, 0);
        } else if (state.find("syncing") != std::string::npos) {
            lv_obj_set_style_text_color(backup_label_, HUD_INFO_COLOR, 0);
        } else if (state.find("failed") != std::string::npos) {
            lv_obj_set_style_text_color(backup_label_, lv_color_hex(0xF44336), 0);
        } else {
            lv_obj_set_style_text_color(backup_label_, HUD_TEXT_COLOR, 0);
        }
    }
}

void HUDOverlay::setAIState(const std::string& state) {
    if (ai_label_) {
        lv_label_set_text(ai_label_, state.c_str());

        // Color code based on state
        if (state.find("on") != std::string::npos) {
            lv_obj_set_style_text_color(ai_label_, lv_color_hex(0x9C27B0), 0);  // Purple
        } else {
            lv_obj_set_style_text_color(ai_label_, lv_color_hex(0x9E9E9E), 0);  // Gray
        }
    }
}

void HUDOverlay::show() {
    if (overlay_) {
        lv_obj_clear_flag(overlay_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(overlay_);
        visible_ = true;
        ESP_LOGD(TAG, "HUD shown");
    }
}

void HUDOverlay::hide() {
    if (overlay_) {
        lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);
        visible_ = false;
        ESP_LOGD(TAG, "HUD hidden");
    }
}
