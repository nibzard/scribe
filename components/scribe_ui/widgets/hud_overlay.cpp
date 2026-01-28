#include "hud_overlay.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>
#include <string>

static const char* TAG = "SCRIBE_HUD";

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
        lv_obj_delete(overlay_);
        overlay_ = nullptr;
    }
}

void HUDOverlay::createWidgets() {
    const Theme::Colors& colors = Theme::getColors();
    // Create main container (semi-transparent dark box)
    container_ = lv_obj_create(overlay_);
    lv_obj_set_size(container_, 280, 180);
    lv_obj_center(container_);
    lv_obj_set_style_bg_color(container_, colors.fg, 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_90, 0);
    lv_obj_set_style_border_width(container_, 2, 0);
    lv_obj_set_style_border_color(container_, colors.border, 0);
    lv_obj_set_style_border_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(container_, 10, 0);
    lv_obj_set_style_pad_all(container_, 15, 0);

    // Project name label (title)
    project_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(project_label_, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(project_label_, colors.text, 0);
    lv_label_set_long_mode(project_label_, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(project_label_, 250);
    lv_obj_align(project_label_, LV_ALIGN_TOP_MID, 0, 0);

    // Separator line
    separator_ = lv_obj_create(container_);
    lv_obj_set_size(separator_, 250, 1);
    lv_obj_set_style_bg_color(separator_, colors.border, 0);
    lv_obj_set_style_bg_opa(separator_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(separator_, 0, 0);
    lv_obj_align(separator_, LV_ALIGN_TOP_MID, 0, 30);

    // Word counts section
    words_today_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(words_today_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(words_today_label_, colors.text, 0);
    lv_obj_align(words_today_label_, LV_ALIGN_TOP_LEFT, 0, 40);
    {
        std::string text = std::string(Strings::getInstance().get("hud.words_today")) + ": 0";
        lv_label_set_text(words_today_label_, text.c_str());
    }

    words_total_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(words_total_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(words_total_label_, colors.text, 0);
    lv_obj_align(words_total_label_, LV_ALIGN_TOP_LEFT, 140, 40);
    {
        std::string text = std::string(Strings::getInstance().get("hud.words_total")) + ": 0";
        lv_label_set_text(words_total_label_, text.c_str());
    }

    // Battery label
    battery_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(battery_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(battery_label_, colors.text, 0);
    lv_obj_align(battery_label_, LV_ALIGN_TOP_LEFT, 0, 70);
    {
        std::string text = std::string(Strings::getInstance().get("hud.battery")) + ": --%";
        lv_label_set_text(battery_label_, text.c_str());
    }

    // Save state label
    save_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(save_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(save_label_, colors.success, 0);
    lv_obj_align(save_label_, LV_ALIGN_TOP_LEFT, 0, 100);
    lv_label_set_text(save_label_, Strings::getInstance().get("hud.saved"));

    // Backup state label
    backup_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(backup_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(backup_label_, colors.info, 0);
    lv_obj_align(backup_label_, LV_ALIGN_TOP_LEFT, 0, 125);
    lv_label_set_text(backup_label_, Strings::getInstance().get("hud.backup_off"));

    // AI state label
    ai_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(ai_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ai_label_, colors.text_secondary, 0);
    lv_obj_align(ai_label_, LV_ALIGN_TOP_LEFT, 140, 125);
    lv_label_set_text(ai_label_, Strings::getInstance().get("hud.ai_off"));

    ESP_LOGI(TAG, "HUD widgets created");
}

void HUDOverlay::setProjectName(const std::string& name) {
    if (project_label_) {
        lv_label_set_text_fmt(project_label_, "%s", name.c_str());
    }
}

void HUDOverlay::setWordCounts(size_t today, size_t total) {
    if (words_today_label_) {
        std::string text = std::string(Strings::getInstance().get("hud.words_today")) + ": " +
                           std::to_string(today);
        lv_label_set_text(words_today_label_, text.c_str());
    }
    if (words_total_label_) {
        std::string text = std::string(Strings::getInstance().get("hud.words_total")) + ": " +
                           std::to_string(total);
        lv_label_set_text(words_total_label_, text.c_str());
    }
}

void HUDOverlay::setBattery(int percentage, bool charging) {
    if (battery_label_) {
        const Theme::Colors& colors = Theme::getColors();
        const char* icon = charging ? "\u26A1" : "";
        std::string text = std::string(icon) + " " +
                           Strings::getInstance().get("hud.battery") + ": " +
                           std::to_string(percentage) + "%";
        lv_label_set_text(battery_label_, text.c_str());

        // Color code battery level
        lv_color_t color = colors.text;
        if (percentage <= 20) {
            color = colors.error;
        } else if (percentage <= 50) {
            color = colors.warning;
        }
        lv_obj_set_style_text_color(battery_label_, color, 0);
    }
}

void HUDOverlay::setSaveState(const std::string& state) {
    if (save_label_) {
        const Theme::Colors& colors = Theme::getColors();
        lv_label_set_text(save_label_, state.c_str());

        // Color code based on state
        lv_color_t color = colors.text;
        if (state.find("Saved") != std::string::npos ||
            state.find("\u2713") != std::string::npos) {
            color = colors.success;
        } else if (state.find("Saving") != std::string::npos ||
                   state.find("\u2026") != std::string::npos) {
            color = colors.warning;
        }
        lv_obj_set_style_text_color(save_label_, color, 0);
    }
}

void HUDOverlay::setBackupState(const std::string& state) {
    if (backup_label_) {
        const Theme::Colors& colors = Theme::getColors();
        lv_label_set_text(backup_label_, state.c_str());

        // Show/hide and color based on state
        if (state.find("off") != std::string::npos) {
            lv_obj_set_style_text_color(backup_label_, colors.text_secondary, 0);
        } else if (state.find("synced") != std::string::npos ||
                   state.find("\u2713") != std::string::npos) {
            lv_obj_set_style_text_color(backup_label_, colors.success, 0);
        } else if (state.find("syncing") != std::string::npos) {
            lv_obj_set_style_text_color(backup_label_, colors.info, 0);
        } else if (state.find("failed") != std::string::npos) {
            lv_obj_set_style_text_color(backup_label_, colors.error, 0);
        } else {
            lv_obj_set_style_text_color(backup_label_, colors.text, 0);
        }
    }
}

void HUDOverlay::setAIState(const std::string& state) {
    if (ai_label_) {
        const Theme::Colors& colors = Theme::getColors();
        lv_label_set_text(ai_label_, state.c_str());

        // Color code based on state
        if (state.find("on") != std::string::npos) {
            lv_obj_set_style_text_color(ai_label_, colors.accent, 0);
        } else {
            lv_obj_set_style_text_color(ai_label_, colors.text_secondary, 0);
        }
    }
}

void HUDOverlay::show() {
    if (overlay_) {
        applyTheme();
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

void HUDOverlay::applyTheme() {
    if (!container_) {
        return;
    }
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(container_, colors.fg, 0);
    lv_obj_set_style_border_color(container_, colors.border, 0);
    if (separator_) {
        lv_obj_set_style_bg_color(separator_, colors.border, 0);
    }
    if (project_label_) lv_obj_set_style_text_color(project_label_, colors.text, 0);
    if (words_today_label_) lv_obj_set_style_text_color(words_today_label_, colors.text, 0);
    if (words_total_label_) lv_obj_set_style_text_color(words_total_label_, colors.text, 0);
    if (battery_label_) lv_obj_set_style_text_color(battery_label_, colors.text, 0);
    if (save_label_) lv_obj_set_style_text_color(save_label_, colors.success, 0);
    if (backup_label_) lv_obj_set_style_text_color(backup_label_, colors.info, 0);
    if (ai_label_) lv_obj_set_style_text_color(ai_label_, colors.text_secondary, 0);
}
