#include "screen_export.h"
#include "../../scribe_utils/strings.h"
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
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenExport::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("export.title"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_20, 0);

    // Export options list
    btn_list_ = lv_list_create(screen_);
    lv_obj_set_size(btn_list_, 300, 250);
    lv_obj_align(btn_list_, LV_ALIGN_TOP_MID, 0, 90);

    buttons_.clear();
    types_.clear();

    // Send to Computer (highlighted - recommended)
    buttons_.push_back(lv_list_add_button(btn_list_, LV_SYMBOL_WIFI,
        Strings::getInstance().get("export.send_to_computer")));
    types_.push_back("send_to_computer");

    // Export to SD - .md (default)
    buttons_.push_back(lv_list_add_button(btn_list_, LV_SYMBOL_SD_CARD,
        Strings::getInstance().get("export.to_sd_md")));
    types_.push_back("sd_md");

    // Export to SD - .txt
    buttons_.push_back(lv_list_add_button(btn_list_, LV_SYMBOL_FILE,
        Strings::getInstance().get("export.to_sd_txt")));
    types_.push_back("sd_txt");

    // Privacy notice
    privacy_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(privacy_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(privacy_label_, 300);
    lv_label_set_text(privacy_label_, Strings::getInstance().get("export.privacy"));
    lv_obj_align(privacy_label_, LV_ALIGN_TOP_MID, 0, LV_VER_RES - 80);

    // Progress label (hidden initially)
    progress_label_ = lv_label_create(screen_);
    lv_label_set_text(progress_label_, "");
    lv_obj_align(progress_label_, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);

    // Send to Computer instructions (hidden by default)
    instructions_body_ = lv_label_create(screen_);
    lv_label_set_long_mode(instructions_body_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(instructions_body_, 320);
    lv_label_set_text(instructions_body_, Strings::getInstance().get("export.send_instructions_body"));
    lv_obj_align(instructions_body_, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_style_text_align(instructions_body_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(instructions_body_, LV_OBJ_FLAG_HIDDEN);

    instructions_confirm_ = lv_label_create(screen_);
    lv_label_set_text(instructions_confirm_, Strings::getInstance().get("export.send_confirm"));
    lv_obj_align(instructions_confirm_, LV_ALIGN_TOP_MID, 0, 200);
    lv_obj_add_flag(instructions_confirm_, LV_OBJ_FLAG_HIDDEN);

    instructions_cancel_ = lv_label_create(screen_);
    lv_label_set_text(instructions_cancel_, Strings::getInstance().get("export.send_cancel"));
    lv_obj_align(instructions_cancel_, LV_ALIGN_TOP_MID, 0, 220);
    lv_obj_add_flag(instructions_cancel_, LV_OBJ_FLAG_HIDDEN);
}

void ScreenExport::show() {
    if (screen_) {
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
