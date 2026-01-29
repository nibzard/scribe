#include "screen_editor.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_EDITOR";

ScreenEditor::ScreenEditor() : screen_(nullptr), text_view_(nullptr), hud_panel_(nullptr) {
    text_margin_px_ = Theme::scalePx(20);
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
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenEditor::createWidgets() {
    // Main text view
    text_view_ = new TextView(screen_);
    if (text_view_) {
        lv_obj_set_size(text_view_->obj(), LV_HOR_RES, LV_VER_RES);
        lv_obj_align(text_view_->obj(), LV_ALIGN_TOP_LEFT, 0, 0);
        updateTextViewLayout();
    }

    // HUD panel (hidden by default)
    hud_panel_ = lv_obj_create(screen_);
    lv_obj_set_size(hud_panel_, Theme::scalePx(240), Theme::scalePx(180));
    lv_obj_align(hud_panel_, LV_ALIGN_TOP_RIGHT, -Theme::scalePx(10), Theme::scalePx(10));
    lv_obj_add_flag(hud_panel_, LV_OBJ_FLAG_HIDDEN);
    applyTheme();

    // HUD labels (left) and values (right)
    int y = Theme::scalePx(10);
    int line_gap = Theme::scalePx(20);
    int section_gap = Theme::scalePx(22);
    hud_project_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_project_label_, Strings::getInstance().get("hud.project"));
    lv_obj_align(hud_project_label_, LV_ALIGN_TOP_LEFT, Theme::scalePx(10), y);

    hud_project_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_project_value_, "");
    lv_obj_align(hud_project_value_, LV_ALIGN_TOP_RIGHT, -Theme::scalePx(10), y);

    y += line_gap;
    hud_today_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_today_label_, Strings::getInstance().get("hud.words_today"));
    lv_obj_align(hud_today_label_, LV_ALIGN_TOP_LEFT, Theme::scalePx(10), y);

    hud_today_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_today_value_, "0");
    lv_obj_align(hud_today_value_, LV_ALIGN_TOP_RIGHT, -Theme::scalePx(10), y);

    y += line_gap;
    hud_total_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_total_label_, Strings::getInstance().get("hud.words_total"));
    lv_obj_align(hud_total_label_, LV_ALIGN_TOP_LEFT, Theme::scalePx(10), y);

    hud_total_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_total_value_, "0");
    lv_obj_align(hud_total_value_, LV_ALIGN_TOP_RIGHT, -Theme::scalePx(10), y);

    y += line_gap;
    hud_battery_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_battery_label_, Strings::getInstance().get("hud.battery"));
    lv_obj_align(hud_battery_label_, LV_ALIGN_TOP_LEFT, Theme::scalePx(10), y);

    hud_battery_value_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_battery_value_, "0%");
    lv_obj_align(hud_battery_value_, LV_ALIGN_TOP_RIGHT, -Theme::scalePx(10), y);

    y += section_gap;
    hud_save_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_save_label_, Strings::getInstance().get("hud.saved"));
    lv_obj_align(hud_save_label_, LV_ALIGN_TOP_LEFT, Theme::scalePx(10), y);

    y += line_gap;
    hud_backup_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_backup_label_, Strings::getInstance().get("hud.backup_off"));
    lv_obj_align(hud_backup_label_, LV_ALIGN_TOP_LEFT, Theme::scalePx(10), y);

    y += line_gap;
    hud_ai_label_ = lv_label_create(hud_panel_);
    lv_label_set_text(hud_ai_label_, Strings::getInstance().get("hud.ai_off"));
    lv_obj_align(hud_ai_label_, LV_ALIGN_TOP_LEFT, Theme::scalePx(10), y);
}

void ScreenEditor::show() {
    if (screen_) {
        applyTheme();
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
    const Theme::Colors& colors = Theme::getColors();
    if (hud_panel_) {
        lv_obj_set_style_bg_color(hud_panel_, colors.fg, 0);
        lv_obj_set_style_bg_opa(hud_panel_, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(hud_panel_, colors.border, 0);
        lv_obj_set_style_border_width(hud_panel_, 1, 0);
    }
    if (hud_project_label_) lv_obj_set_style_text_color(hud_project_label_, colors.text_secondary, 0);
    if (hud_project_value_) lv_obj_set_style_text_color(hud_project_value_, colors.text, 0);
    if (hud_today_label_) lv_obj_set_style_text_color(hud_today_label_, colors.text_secondary, 0);
    if (hud_today_value_) lv_obj_set_style_text_color(hud_today_value_, colors.text, 0);
    if (hud_total_label_) lv_obj_set_style_text_color(hud_total_label_, colors.text_secondary, 0);
    if (hud_total_value_) lv_obj_set_style_text_color(hud_total_value_, colors.text, 0);
    if (hud_battery_label_) lv_obj_set_style_text_color(hud_battery_label_, colors.text_secondary, 0);
    if (hud_battery_value_) lv_obj_set_style_text_color(hud_battery_value_, colors.text, 0);
    if (hud_save_label_) lv_obj_set_style_text_color(hud_save_label_, colors.text, 0);
    if (hud_backup_label_) lv_obj_set_style_text_color(hud_backup_label_, colors.text, 0);
    if (hud_ai_label_) lv_obj_set_style_text_color(hud_ai_label_, colors.text, 0);
}

void ScreenEditor::setEditorFont(const lv_font_t* font) {
    if (text_view_ && font) {
        text_view_->setFont(font);
    }
}

void ScreenEditor::setEditorMargin(int margin_px) {
    if (margin_px < 0) {
        margin_px = 0;
    }
    text_margin_px_ = margin_px;
    updateTextViewLayout();
}

void ScreenEditor::handleDisplayResize() {
    if (!screen_) {
        return;
    }
    applyTheme();
}

void ScreenEditor::applyTheme() {
    if (screen_) {
        lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    }
    if (text_view_) {
        lv_obj_set_size(text_view_->obj(), LV_HOR_RES, LV_VER_RES);
        lv_obj_align(text_view_->obj(), LV_ALIGN_TOP_LEFT, 0, 0);
    }
    Theme::applyScreenStyle(screen_);
    if (text_view_) {
        text_view_->applyTheme();
        updateTextViewLayout();
    }
    if (hud_panel_) {
        const Theme::Colors& colors = Theme::getColors();
        lv_obj_set_size(hud_panel_, Theme::scalePx(240), Theme::scalePx(180));
        lv_obj_align(hud_panel_, LV_ALIGN_TOP_RIGHT, -Theme::scalePx(10), Theme::scalePx(10));
        lv_obj_set_style_bg_color(hud_panel_, colors.fg, 0);
        lv_obj_set_style_bg_opa(hud_panel_, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(hud_panel_, colors.border, 0);
        lv_obj_set_style_border_width(hud_panel_, 1, 0);
        lv_obj_set_style_text_font(hud_panel_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

        int y = Theme::scalePx(10);
        int line_gap = Theme::scalePx(20);
        int section_gap = Theme::scalePx(22);
        int x_left = Theme::scalePx(10);
        int x_right = -Theme::scalePx(10);
        if (hud_project_label_) lv_obj_align(hud_project_label_, LV_ALIGN_TOP_LEFT, x_left, y);
        if (hud_project_value_) lv_obj_align(hud_project_value_, LV_ALIGN_TOP_RIGHT, x_right, y);
        y += line_gap;
        if (hud_today_label_) lv_obj_align(hud_today_label_, LV_ALIGN_TOP_LEFT, x_left, y);
        if (hud_today_value_) lv_obj_align(hud_today_value_, LV_ALIGN_TOP_RIGHT, x_right, y);
        y += line_gap;
        if (hud_total_label_) lv_obj_align(hud_total_label_, LV_ALIGN_TOP_LEFT, x_left, y);
        if (hud_total_value_) lv_obj_align(hud_total_value_, LV_ALIGN_TOP_RIGHT, x_right, y);
        y += line_gap;
        if (hud_battery_label_) lv_obj_align(hud_battery_label_, LV_ALIGN_TOP_LEFT, x_left, y);
        if (hud_battery_value_) lv_obj_align(hud_battery_value_, LV_ALIGN_TOP_RIGHT, x_right, y);
        y += section_gap;
        if (hud_save_label_) lv_obj_align(hud_save_label_, LV_ALIGN_TOP_LEFT, x_left, y);
        y += line_gap;
        if (hud_backup_label_) lv_obj_align(hud_backup_label_, LV_ALIGN_TOP_LEFT, x_left, y);
        y += line_gap;
        if (hud_ai_label_) lv_obj_align(hud_ai_label_, LV_ALIGN_TOP_LEFT, x_left, y);
    }
}

void ScreenEditor::updateTextViewLayout() {
    if (!text_view_) {
        return;
    }
    lv_obj_update_layout(text_view_->obj());
    lv_area_t coords;
    lv_obj_get_coords(text_view_->obj(), &coords);
    int width = lv_area_get_width(&coords);
    int height = lv_area_get_height(&coords);
    int margin = text_margin_px_;
    if (margin < 0) {
        margin = 0;
    }
    int inner_width = width - (margin * 2);
    int inner_height = height - (margin * 2);
    if (inner_width < 1) inner_width = 1;
    if (inner_height < 1) inner_height = 1;
    text_view_->setContentInsets(margin, margin, margin, margin);
    text_view_->setViewportSize(inner_width, inner_height);
}
