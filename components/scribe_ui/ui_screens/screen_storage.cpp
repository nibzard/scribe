#include "screen_storage.h"
#include "../../scribe_storage/storage_manager.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>
#include <cstdio>

static const char* TAG = "SCRIBE_SCREEN_STORAGE";

ScreenStorage::ScreenStorage() : screen_(nullptr), list_(nullptr), status_label_(nullptr), detail_label_(nullptr) {
}

ScreenStorage::~ScreenStorage() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenStorage::init() {
    ESP_LOGI(TAG, "Initializing storage screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenStorage::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("storage.title"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(20));
    lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);

    // Status
    status_label_ = lv_label_create(screen_);
    lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(60));
    lv_obj_set_style_text_font(status_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    detail_label_ = lv_label_create(screen_);
    lv_obj_align(detail_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(90));
    lv_obj_set_style_text_font(detail_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);

    // Action list
    list_ = lv_list_create(screen_);
    lv_obj_set_size(list_, Theme::fitWidth(360, 40), Theme::fitHeight(260, 220));
    lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(140));
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(list_, colors.fg, 0);
    lv_obj_set_style_bg_opa(list_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(list_, colors.border, 0);
    lv_obj_set_style_border_width(list_, 1, 0);
    lv_obj_set_style_text_font(list_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    buttons_.clear();
    item_keys_.clear();

    auto addItem = [&](const char* label, const char* key, const char* symbol) {
        lv_obj_t* btn = lv_list_add_button(list_, symbol, label);
        lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
        buttons_.push_back(btn);
        item_keys_.push_back(key);
    };

    Strings& strings = Strings::getInstance();
    addItem(strings.get("storage.format"), "format", LV_SYMBOL_SD_CARD);
    addItem(strings.get("common.back"), "back", LV_SYMBOL_LEFT);

    selected_index_ = 0;
    updateSelection();

    // Busy overlay (format progress)
    busy_overlay_ = lv_obj_create(screen_);
    lv_obj_set_size(busy_overlay_, Theme::fitWidth(360, 40), Theme::fitHeight(220, 220));
    lv_obj_center(busy_overlay_);
    lv_obj_set_style_bg_opa(busy_overlay_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(busy_overlay_, 1, 0);
    lv_obj_set_style_text_font(busy_overlay_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    busy_spinner_ = lv_spinner_create(busy_overlay_);
    lv_spinner_set_anim_params(busy_spinner_, 1000, 60);
    lv_obj_set_size(busy_spinner_, Theme::scalePx(48), Theme::scalePx(48));
    lv_obj_align(busy_spinner_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(15));

    busy_label_ = lv_label_create(busy_overlay_);
    lv_label_set_text(busy_label_, Strings::getInstance().get("storage.formatting"));
    lv_obj_align(busy_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(80));
    lv_obj_set_style_text_font(busy_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    busy_detail_ = lv_label_create(busy_overlay_);
    lv_label_set_text(busy_detail_, Strings::getInstance().get("storage.formatting_detail"));
    lv_obj_align(busy_detail_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(115));
    lv_obj_set_width(busy_detail_, Theme::scalePx(300));
    lv_label_set_long_mode(busy_detail_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(busy_detail_, Theme::getUIFont(Theme::UiFontRole::Small), 0);

    lv_obj_add_flag(busy_overlay_, LV_OBJ_FLAG_HIDDEN);
}

void ScreenStorage::show() {
    if (!screen_) return;

    Theme::applyScreenStyle(screen_);
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_text_font(screen_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
    if (title_label_) {
        lv_obj_set_style_text_color(title_label_, colors.text, 0);
        lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
        lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(20));
    }
    if (status_label_) {
        lv_obj_set_style_text_color(status_label_, colors.text, 0);
        lv_obj_set_style_text_font(status_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(60));
    }
    if (detail_label_) {
        lv_obj_set_style_text_color(detail_label_, colors.text_secondary, 0);
        lv_obj_set_style_text_font(detail_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
        lv_obj_align(detail_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(90));
    }
    if (list_) {
        lv_obj_set_size(list_, Theme::fitWidth(360, 40), Theme::fitHeight(260, 220));
        lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(140));
        lv_obj_set_style_bg_color(list_, colors.fg, 0);
        lv_obj_set_style_bg_opa(list_, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(list_, colors.border, 0);
        lv_obj_set_style_border_width(list_, 1, 0);
        lv_obj_set_style_text_font(list_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
    }
    for (auto* btn : buttons_) {
        if (!btn) continue;
        lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
    }

    updateBusyStyle();
    updateStatus();
    updateSelection();
    lv_screen_load(screen_);

    if (prompt_on_show_) {
        prompt_on_show_ = false;
        showConfirmDialog();
    }
}

void ScreenStorage::hide() {
    if (back_cb_) {
        back_cb_();
    }
}

void ScreenStorage::moveSelection(int delta) {
    if (busy_ || confirm_dialog_) return;
    int count = static_cast<int>(buttons_.size());
    if (count <= 0) return;

    selected_index_ += delta;
    if (selected_index_ < 0) selected_index_ = count - 1;
    if (selected_index_ >= count) selected_index_ = 0;

    updateSelection();
}

void ScreenStorage::selectCurrent() {
    if (busy_) return;
    if (confirm_dialog_) {
        confirmFormat();
        return;
    }

    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(item_keys_.size())) {
        return;
    }
    const std::string& key = item_keys_[selected_index_];
    if (key == "back") {
        if (back_cb_) back_cb_();
        return;
    }
    if (key == "format") {
        showConfirmDialog();
        return;
    }
}

void ScreenStorage::updateSelection() {
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (static_cast<int>(i) == selected_index_) {
            lv_obj_add_state(buttons_[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(buttons_[i], LV_STATE_CHECKED);
        }
    }
    if (list_) {
        lv_obj_invalidate(list_);
    }
}

void ScreenStorage::updateStatus() {
    StorageManager& storage = StorageManager::getInstance();
    Strings& strings = Strings::getInstance();
    const Theme::Colors& colors = Theme::getColors();

    if (status_label_) {
        lv_obj_set_style_text_color(status_label_, colors.text, 0);
    }
    if (detail_label_) {
        lv_obj_set_style_text_color(detail_label_, colors.text_secondary, 0);
    }

    if (storage.isMounted()) {
        if (status_label_) {
            lv_label_set_text(status_label_, strings.get("storage.status_ready"));
        }
        char buf[64];
        size_t free_mb = storage.getFreeSpace() / (1024U * 1024U);
        snprintf(buf, sizeof(buf), strings.get("storage.free_space_fmt"), static_cast<unsigned long>(free_mb));
        if (detail_label_) {
            lv_label_set_text(detail_label_, buf);
        }
    } else {
        if (status_label_) {
            lv_label_set_text(status_label_, strings.get("storage.status_unavailable"));
        }
        if (detail_label_) {
            lv_label_set_text(detail_label_, strings.get("storage.requires_fat32"));
        }
    }
}

void ScreenStorage::setBusy(bool busy, const char* status, const char* detail) {
    busy_ = busy;
    if (!busy_overlay_) {
        return;
    }

    if (busy) {
        hideConfirmDialog();
        lv_obj_clear_flag(busy_overlay_, LV_OBJ_FLAG_HIDDEN);
        if (list_) {
            lv_obj_add_state(list_, LV_STATE_DISABLED);
        }
        if (status && busy_label_) {
            lv_label_set_text(busy_label_, status);
        }
        if (detail && busy_detail_) {
            lv_label_set_text(busy_detail_, detail);
        }
    } else {
        lv_obj_add_flag(busy_overlay_, LV_OBJ_FLAG_HIDDEN);
        if (list_) {
            lv_obj_clear_state(list_, LV_STATE_DISABLED);
        }
    }
}

void ScreenStorage::setStatusText(const char* status, const char* detail) {
    if (status_label_ && status) {
        lv_label_set_text(status_label_, status);
    }
    if (detail_label_ && detail) {
        lv_label_set_text(detail_label_, detail);
    }
}

void ScreenStorage::refreshStatus() {
    updateStatus();
}

void ScreenStorage::showConfirmDialog() {
    if (confirm_dialog_) {
        return;
    }

    confirm_step_ = 1;
    confirm_dialog_ = lv_obj_create(screen_);
    lv_obj_set_size(confirm_dialog_, Theme::fitWidth(360, 40), Theme::fitHeight(200, 200));
    lv_obj_center(confirm_dialog_);
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(confirm_dialog_, colors.fg, 0);
    lv_obj_set_style_bg_opa(confirm_dialog_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(confirm_dialog_, colors.border, 0);
    lv_obj_set_style_border_width(confirm_dialog_, 1, 0);
    lv_obj_set_style_text_font(confirm_dialog_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    lv_obj_t* title = lv_label_create(confirm_dialog_);
    lv_label_set_text(title, Strings::getInstance().get("storage.format_title"));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, Theme::scalePx(15));
    lv_obj_set_style_text_font(title, Theme::getUIFont(Theme::UiFontRole::Title), 0);

    confirm_body_ = lv_label_create(confirm_dialog_);
    lv_label_set_text(confirm_body_, Strings::getInstance().get("storage.format_step1"));
    lv_obj_align(confirm_body_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(60));
    lv_obj_set_width(confirm_body_, Theme::scalePx(320));
    lv_label_set_long_mode(confirm_body_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(confirm_body_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    lv_obj_t* hint = lv_label_create(confirm_dialog_);
    lv_label_set_text(hint, Strings::getInstance().get("storage.format_hint"));
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(15));
    lv_obj_set_style_text_font(hint, Theme::getUIFont(Theme::UiFontRole::Small), 0);
}

void ScreenStorage::hideConfirmDialog() {
    if (confirm_dialog_) {
        lv_obj_delete(confirm_dialog_);
        confirm_dialog_ = nullptr;
    }
    confirm_body_ = nullptr;
    confirm_step_ = 0;
}

void ScreenStorage::confirmFormat() {
    if (!confirm_dialog_) {
        return;
    }

    if (confirm_step_ == 1) {
        confirm_step_ = 2;
        if (confirm_body_) {
            lv_label_set_text(confirm_body_, Strings::getInstance().get("storage.format_step2"));
        }
        return;
    }

    hideConfirmDialog();
    if (format_cb_) {
        format_cb_();
    }
    updateStatus();
}

void ScreenStorage::cancelConfirm() {
    hideConfirmDialog();
}

void ScreenStorage::updateBusyStyle() {
    if (!busy_overlay_) return;
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_size(busy_overlay_, Theme::fitWidth(360, 40), Theme::fitHeight(220, 220));
    lv_obj_center(busy_overlay_);
    lv_obj_set_style_bg_color(busy_overlay_, colors.fg, 0);
    lv_obj_set_style_border_color(busy_overlay_, colors.border, 0);
    if (busy_spinner_) {
        lv_obj_set_size(busy_spinner_, Theme::scalePx(48), Theme::scalePx(48));
        lv_obj_align(busy_spinner_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(15));
    }
    if (busy_label_) {
        lv_obj_set_style_text_color(busy_label_, colors.text, 0);
        lv_obj_set_style_text_font(busy_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        lv_obj_align(busy_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(80));
    }
    if (busy_detail_) {
        lv_obj_set_style_text_color(busy_detail_, colors.text_secondary, 0);
        lv_obj_set_style_text_font(busy_detail_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
        lv_obj_align(busy_detail_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(115));
        lv_obj_set_width(busy_detail_, Theme::scalePx(300));
    }
}
