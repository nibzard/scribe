#include "screen_editor.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_EDITOR";

ScreenEditor::ScreenEditor() : screen_(nullptr), text_area_(nullptr), hud_panel_(nullptr) {
}

ScreenEditor::~ScreenEditor() {
    if (screen_) {
        lv_obj_del(screen_);
    }
}

void ScreenEditor::init() {
    ESP_LOGI(TAG, "Initializing editor screen");

    // Create new screen
    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenEditor::createWidgets() {
    // Main text area
    text_area_ = lv_textarea_create(screen_);
    lv_obj_set_size(text_area_, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(text_area_, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_textarea_set_cursor_hidden(text_area_, false);
    lv_obj_set_style_bg_opa(text_area_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(text_area_, LV_OPA_TRANSP, 0);

    // HUD panel (hidden by default)
    hud_panel_ = lv_obj_create(screen_);
    lv_obj_set_size(hud_panel_, 240, 180);
    lv_obj_align(hud_panel_, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_flag(hud_panel_, LV_OBJ_FLAG_HIDDEN);

    // HUD labels (left) and values (right)
    int y = 10;
    hud_project_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_project_label_, "Project");
    lv_obj_align(hud_project_label_, LV_ALIGN_TOP_LEFT, 10, y);

    hud_project_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_project_value_, "");
    lv_obj_align(hud_project_value_, LV_ALIGN_TOP_RIGHT, -10, y);

    y += 20;
    hud_today_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_today_label_, "Today");
    lv_obj_align(hud_today_label_, LV_ALIGN_TOP_LEFT, 10, y);

    hud_today_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_today_value_, "0");
    lv_obj_align(hud_today_value_, LV_ALIGN_TOP_RIGHT, -10, y);

    y += 20;
    hud_total_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_total_label_, "Total");
    lv_obj_align(hud_total_label_, LV_ALIGN_TOP_LEFT, 10, y);

    hud_total_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_total_value_, "0");
    lv_obj_align(hud_total_value_, LV_ALIGN_TOP_RIGHT, -10, y);

    y += 20;
    hud_battery_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_battery_label_, "Battery");
    lv_obj_align(hud_battery_label_, LV_ALIGN_TOP_LEFT, 10, y);

    hud_battery_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_battery_value_, "0%");
    lv_obj_align(hud_battery_value_, LV_ALIGN_TOP_RIGHT, -10, y);

    y += 22;
    hud_save_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_save_label_, "Saved \u2713");
    lv_obj_align(hud_save_label_, LV_ALIGN_TOP_LEFT, 10, y);

    y += 20;
    hud_backup_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_backup_label_, "Backup: off");
    lv_obj_align(hud_backup_label_, LV_ALIGN_TOP_LEFT, 10, y);

    y += 20;
    hud_ai_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_ai_label_, "AI: off");
    lv_obj_align(hud_ai_label_, LV_ALIGN_TOP_LEFT, 10, y);
}

void ScreenEditor::show() {
    if (screen_) {
        lv_scr_load(screen_);
    }
}

void ScreenEditor::hide() {
    // Screen is hidden when another screen is loaded
}

void ScreenEditor::loadContent(const std::string& content) {
    if (text_area_) {
        lv_textarea_set_text(text_area_, content.c_str());
    }
}

std::string ScreenEditor::getContent() const {
    if (text_area_) {
        const char* text = lv_textarea_get_text(text_area_);
        return std::string(text ? text : "");
    }
    return "";
}

void ScreenEditor::update() {
    if (!editor_ || !text_area_) {
        return;
    }

    std::string content = editor_->getText();
    if (content != cached_text_) {
        cached_text_ = content;
        lv_textarea_set_text(text_area_, cached_text_.c_str());
    }

    lv_textarea_set_cursor_pos(text_area_, editor_->getCursor().pos);
    lv_obj_invalidate(text_area_);
    if (hud_visible_) {
        updateHUD();
    }
}

void ScreenEditor::showHUD(bool show) {
    if (hud_panel_) {
        if (show) {
            lv_obj_clear_flag(hud_panel_, LV_OBJ_FLAG_HIDDEN);
            updateHUD();
        } else {
            lv_obj_add_flag(hud_panel_, LV_OBJ_FLAG_HIDDEN);
        }
        hud_visible_ = show;
    }
}

void ScreenEditor::setHUDProjectName(const std::string& name) {
    if (hud_project_value_) {
        lv_label_set_text(hud_project_value_, name.c_str());
    }
}

void ScreenEditor::setHUDWordCounts(size_t today, size_t total) {
    if (hud_today_value_) {
        lv_label_set_text_fmt(hud_today_value_, "%zu", today);
    }
    if (hud_total_value_) {
        lv_label_set_text_fmt(hud_total_value_, "%zu", total);
    }
}

void ScreenEditor::setHUDBattery(int percentage, bool charging) {
    if (hud_battery_value_) {
        lv_label_set_text_fmt(hud_battery_value_, "%d%%%s", percentage, charging ? "+" : "");
    }
}

void ScreenEditor::setHUDSaveState(const std::string& status) {
    if (hud_save_label_) {
        lv_label_set_text(hud_save_label_, status.c_str());
    }
}

void ScreenEditor::setHUDBackupState(const std::string& status) {
    if (hud_backup_label_) {
        lv_label_set_text(hud_backup_label_, status.c_str());
    }
}

void ScreenEditor::setHUDAIState(const std::string& status) {
    if (hud_ai_label_) {
        lv_label_set_text(hud_ai_label_, status.c_str());
    }
}

void ScreenEditor::updateHUD() {
    if (!editor_ || !hud_panel_) return;
}
