#include "screen_export.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_EXPORT";

ScreenExport::ScreenExport() : screen_(nullptr), selected_index_(0) {
}

ScreenExport::~ScreenExport() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenExport::init() {
    ESP_LOGI(TAG, "Initializing export screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenExport::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("export.title"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(40));
    lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);

    // Export options list
    btn_list_ = lv_list_create(screen_);
    lv_obj_set_size(btn_list_, Theme::fitWidth(300, 40), Theme::fitHeight(250, 200));
    lv_obj_align(btn_list_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(90));
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(btn_list_, colors.fg, 0);
    lv_obj_set_style_bg_opa(btn_list_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn_list_, colors.border, 0);
    lv_obj_set_style_border_width(btn_list_, 1, 0);
    lv_obj_set_style_text_font(btn_list_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    buttons_.clear();
    types_.clear();

    auto addOption = [&](const char* label, const char* type, const char* symbol) {
        lv_obj_t* btn = lv_list_add_button(btn_list_, symbol, label);
        lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
        buttons_.push_back(btn);
        types_.push_back(type);
    };

    // Send to Computer (highlighted - recommended)
    addOption(Strings::getInstance().get("export.send_to_computer"), "send_to_computer", LV_SYMBOL_WIFI);

    // Export to SD - .md (default)
    addOption(Strings::getInstance().get("export.to_sd_md"), "sd_md", LV_SYMBOL_SD_CARD);

    // Export to SD - .txt
    addOption(Strings::getInstance().get("export.to_sd_txt"), "sd_txt", LV_SYMBOL_FILE);

    // Privacy notice
    privacy_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(privacy_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(privacy_label_, Theme::scalePx(300));
    lv_label_set_text(privacy_label_, Strings::getInstance().get("export.privacy"));
    lv_obj_align(privacy_label_, LV_ALIGN_TOP_MID, 0, LV_VER_RES - Theme::scalePx(80));
    lv_obj_set_style_text_color(privacy_label_, colors.text_secondary, 0);
    lv_obj_set_style_text_font(privacy_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);

    // Progress label (hidden initially)
    progress_label_ = lv_label_create(screen_);
    lv_label_set_text(progress_label_, "");
    lv_obj_align(progress_label_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(20));
    lv_obj_add_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(progress_label_, colors.text, 0);
    lv_obj_set_style_text_font(progress_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Send to Computer instructions (hidden by default)
    instructions_body_ = lv_label_create(screen_);
    lv_label_set_long_mode(instructions_body_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(instructions_body_, Theme::fitWidth(320, 40));
    lv_label_set_text(instructions_body_, Strings::getInstance().get("export.send_instructions_body"));
    lv_obj_align(instructions_body_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(120));
    lv_obj_set_style_text_align(instructions_body_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(instructions_body_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(instructions_body_, colors.text, 0);
    lv_obj_set_style_text_font(instructions_body_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    instructions_confirm_ = lv_label_create(screen_);
    lv_label_set_text(instructions_confirm_, Strings::getInstance().get("export.send_confirm"));
    lv_obj_align(instructions_confirm_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(200));
    lv_obj_add_flag(instructions_confirm_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(instructions_confirm_, colors.text_secondary, 0);
    lv_obj_set_style_text_font(instructions_confirm_, Theme::getUIFont(Theme::UiFontRole::Small), 0);

    instructions_cancel_ = lv_label_create(screen_);
    lv_label_set_text(instructions_cancel_, Strings::getInstance().get("export.send_cancel"));
    lv_obj_align(instructions_cancel_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(220));
    lv_obj_add_flag(instructions_cancel_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(instructions_cancel_, colors.text_secondary, 0);
    lv_obj_set_style_text_font(instructions_cancel_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
}

void ScreenExport::show() {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        if (btn_list_) {
            const Theme::Colors& colors = Theme::getColors();
            lv_obj_set_size(btn_list_, Theme::fitWidth(300, 40), Theme::fitHeight(250, 200));
            lv_obj_align(btn_list_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(90));
            lv_obj_set_style_bg_color(btn_list_, colors.fg, 0);
            lv_obj_set_style_bg_opa(btn_list_, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(btn_list_, colors.border, 0);
            lv_obj_set_style_border_width(btn_list_, 1, 0);
            lv_obj_set_style_text_font(btn_list_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            for (auto* btn : buttons_) {
                if (!btn) continue;
                lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
                lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
                lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
                lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
            }
            if (title_label_) {
                lv_obj_set_style_text_color(title_label_, colors.text, 0);
                lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
                lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(40));
            }
            if (privacy_label_) {
                lv_obj_set_style_text_color(privacy_label_, colors.text_secondary, 0);
                lv_obj_set_style_text_font(privacy_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
                lv_obj_set_width(privacy_label_, Theme::scalePx(300));
                lv_obj_align(privacy_label_, LV_ALIGN_TOP_MID, 0, LV_VER_RES - Theme::scalePx(80));
            }
            if (progress_label_) {
                lv_obj_set_style_text_color(progress_label_, colors.text, 0);
                lv_obj_set_style_text_font(progress_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
                lv_obj_align(progress_label_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(20));
            }
            if (instructions_body_) {
                lv_obj_set_style_text_color(instructions_body_, colors.text, 0);
                lv_obj_set_style_text_font(instructions_body_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
                lv_obj_set_width(instructions_body_, Theme::fitWidth(320, 40));
                lv_obj_align(instructions_body_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(120));
            }
            if (instructions_confirm_) {
                lv_obj_set_style_text_color(instructions_confirm_, colors.text_secondary, 0);
                lv_obj_set_style_text_font(instructions_confirm_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
                lv_obj_align(instructions_confirm_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(200));
            }
            if (instructions_cancel_) {
                lv_obj_set_style_text_color(instructions_cancel_, colors.text_secondary, 0);
                lv_obj_set_style_text_font(instructions_cancel_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
                lv_obj_align(instructions_cancel_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(220));
            }
        }
        lv_screen_load(screen_);
        selected_index_ = 0;
        setMode(Mode::List);
    }
}

void ScreenExport::hide() {
    if (close_cb_) {
        close_cb_();
    }
}

void ScreenExport::moveSelection(int delta) {
    int count = static_cast<int>(buttons_.size());
    if (count <= 0) return;

    selected_index_ += delta;

    if (selected_index_ < 0) selected_index_ = count - 1;
    if (selected_index_ >= count) selected_index_ = 0;

    updateSelection();
}

std::string ScreenExport::getSelectedType() const {
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(types_.size())) {
        return "";
    }
    return types_[selected_index_];
}

void ScreenExport::selectCurrent() {
    if (export_cb_) {
        export_cb_(getSelectedType());
    }
}

void ScreenExport::updateProgress() {
    if (progress_label_) {
        lv_label_set_text(progress_label_, Strings::getInstance().get("export.progress"));
        lv_obj_clear_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);
    }
}

void ScreenExport::showComplete(const char* message) {
    if (progress_label_) {
        lv_label_set_text(progress_label_, message);
        lv_obj_clear_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);
    }
}

void ScreenExport::showSendInstructions() {
    setMode(Mode::SendInstructions);
}

void ScreenExport::showSendRunning() {
    setMode(Mode::SendRunning);
    if (progress_label_) {
        lv_label_set_text(progress_label_, Strings::getInstance().get("export.send_running"));
        lv_obj_clear_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);
    }
}

void ScreenExport::showSendDone() {
    setMode(Mode::SendRunning);
    if (progress_label_) {
        lv_label_set_text(progress_label_, Strings::getInstance().get("export.send_done"));
        lv_obj_clear_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);
    }
}

void ScreenExport::updateSelection() {
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (static_cast<int>(i) == selected_index_) {
            lv_obj_add_state(buttons_[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(buttons_[i], LV_STATE_CHECKED);
        }
    }
    lv_obj_invalidate(btn_list_);
}

void ScreenExport::setMode(Mode mode) {
    mode_ = mode;
    if (mode_ == Mode::List) {
        lv_label_set_text(title_label_, Strings::getInstance().get("export.title"));
        lv_obj_clear_flag(btn_list_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(privacy_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(instructions_body_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(instructions_confirm_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(instructions_cancel_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);
        updateSelection();
        return;
    }

    lv_label_set_text(title_label_, Strings::getInstance().get("export.send_instructions_title"));
    lv_obj_add_flag(btn_list_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(privacy_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(instructions_body_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(instructions_confirm_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(instructions_cancel_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);

    if (mode_ == Mode::SendRunning) {
        lv_obj_add_flag(instructions_confirm_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(instructions_cancel_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(instructions_body_, LV_OBJ_FLAG_HIDDEN);
    }
}
