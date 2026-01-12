#include "hud_overlay.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_HUD";

HUDOverlay::HUDOverlay(lv_obj_t* parent) : lv_obj() {
    createWidgets();
}

void HUDOverlay::createWidgets() {
    // TODO: Implement custom LVGL widget
    // Create container and labels for HUD elements
}

void HUDOverlay::setProjectName(const char* name) {
    if (project_label_) {
        lv_label_set_text_fmt(project_label_, "Project: %s", name);
    }
}

void HUDOverlay::setWordCount(size_t today, size_t total) {
    if (words_today_label_) {
        lv_label_set_text_fmt(words_today_label_, "Today: %zu", today);
    }
    if (words_total_label_) {
        lv_label_set_text_fmt(words_total_label_, "Total: %zu", total);
    }
}

void HUDOverlay::setBattery(int percentage, bool charging) {
    if (battery_label_) {
        const char* charging_str = charging ? "+" : "";
        lv_label_set_text_fmt(battery_label_, "Battery: %d%%%s", percentage, charging_str);
    }
}

void HUDOverlay::setSaveState(bool saved, bool backing_up) {
    if (save_label_) {
        if (saved) {
            lv_label_set_text(save_label_, "Saved \u2713");
        } else if (backing_up) {
            lv_label_set_text(save_label_, "Backup: syncing\u2026");
        } else {
            lv_label_set_text(save_label_, "Saving\u2026");
        }
    }
}

void HUDOverlay::show() {
    lv_obj_clear_flag(this, LV_OBJ_FLAG_HIDDEN);
    visible_ = true;
}

void HUDOverlay::hide() {
    lv_obj_add_flag(this, LV_OBJ_FLAG_HIDDEN);
    visible_ = false;
}
