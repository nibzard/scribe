#include "screen_editor.h"
#include "../../scribe_utils/strings.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_EDITOR";

ScreenEditor::ScreenEditor() : screen_(nullptr), text_view_(nullptr), hud_panel_(nullptr) {
}

ScreenEditor::~ScreenEditor() {
    if (text_view_) {
        delete text_view_;
        text_view_ = nullptr;
    }
    if (screen_) {
        lv_obj_delete(screen_);
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
    // Main text view
    text_view_ = new TextView(screen_);
    if (text_view_) {
        lv_obj_set_size(text_view_->obj(), LV_HOR_RES, LV_VER_RES);
        lv_obj_align(text_view_->obj(), LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_update_layout(text_view_->obj());
        lv_area_t coords;
        lv_obj_get_coords(text_view_->obj(), &coords);
        text_view_->setViewportSize(lv_area_get_width(&coords) - 20,
                                    lv_area_get_height(&coords) - 20);
    }

    // HUD panel (hidden by default)
    hud_panel_ = lv_obj_create(screen_);
    lv_obj_set_size(hud_panel_, 240, 180);
    lv_obj_align(hud_panel_, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_flag(hud_panel_, LV_OBJ_FLAG_HIDDEN);

    // HUD labels (left) and values (right)
    int y = 10;
    hud_project_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_project_label_, Strings::getInstance().get("hud.project"));
    lv_obj_align(hud_project_label_, LV_ALIGN_TOP_LEFT, 10, y);

    hud_project_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_project_value_, "");
    lv_obj_align(hud_project_value_, LV_ALIGN_TOP_RIGHT, -10, y);

    y += 20;
    hud_today_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_today_label_, Strings::getInstance().get("hud.words_today"));
    lv_obj_align(hud_today_label_, LV_ALIGN_TOP_LEFT, 10, y);

    hud_today_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_today_value_, "0");
    lv_obj_align(hud_today_value_, LV_ALIGN_TOP_RIGHT, -10, y);

    y += 20;
    hud_total_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_total_label_, Strings::getInstance().get("hud.words_total"));
    lv_obj_align(hud_total_label_, LV_ALIGN_TOP_LEFT, 10, y);

    hud_total_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_total_value_, "0");
    lv_obj_align(hud_total_value_, LV_ALIGN_TOP_RIGHT, -10, y);

    y += 20;
    hud_battery_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_battery_label_, Strings::getInstance().get("hud.battery"));
    lv_obj_align(hud_battery_label_, LV_ALIGN_TOP_LEFT, 10, y);

    hud_battery_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_battery_value_, "0%");
    lv_obj_align(hud_battery_value_, LV_ALIGN_TOP_RIGHT, -10, y);

    y += 22;
    hud_save_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_save_label_, Strings::getInstance().get("hud.saved"));
    lv_obj_align(hud_save_label_, LV_ALIGN_TOP_LEFT, 10, y);

    y += 20;
    hud_backup_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_backup_label_, Strings::getInstance().get("hud.backup_off"));
    lv_obj_align(hud_backup_label_, LV_ALIGN_TOP_LEFT, 10, y);

    y += 20;
    hud_ai_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_ai_label_, Strings::getInstance().get("hud.ai_off"));
    lv_obj_align(hud_ai_label_, LV_ALIGN_TOP_LEFT, 10, y);
}

void ScreenEditor::show() {
    if (screen_) {
        lv_screen_load(screen_);
    }
}

void ScreenEditor::hide() {
    // Screen is hidden when another screen is loaded
}

void ScreenEditor::loadContent(const std::string& content) {
    if (text_view_) {
        text_view_->setText(content);
    }
}

std::string ScreenEditor::getContent() const {
    if (text_view_) {
        return text_view_->getText();
    }
    return "";
}

void ScreenEditor::update() {
    if (!editor_ || !text_view_) {
        return;
    }

    uint64_t revision = editor_->getRevision();
    if (revision != last_revision_) {
        last_revision_ = revision;
        text_view_->setSnapshot(editor_->createRenderSnapshot());
    }

    text_view_->setCursor(editor_->getCursor().pos);
    if (editor_->hasSelection()) {
        const Selection& sel = editor_->getSelection();
        text_view_->setSelection(sel.min(), sel.max());
    } else {
        text_view_->clearSelection();
    }
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
