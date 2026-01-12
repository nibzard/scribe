#include "screen_backup.h"
#include "../../scribe_utils/strings.h"
#include "../scribe_services/github_backup.h"
#include "../scribe_services/wifi_manager.h"
#include "../scribe_secrets/secrets_nvs.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_BACKUP";

// ============================================================================
// ScreenBackup Implementation
// ============================================================================

ScreenBackup::ScreenBackup() : screen_(nullptr) {
}

ScreenBackup::~ScreenBackup() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenBackup::init() {
    ESP_LOGI(TAG, "Initializing backup settings screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenBackup::createWidgets() {
    // Title
    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("backup.title"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_20, 0);

    // Description
    description_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(description_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(description_label_, 350);
    lv_label_set_text(description_label_, Strings::getInstance().get("backup.off_desc"));
    lv_obj_align(description_label_, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_text_align(description_label_, LV_TEXT_ALIGN_CENTER, 0);

    // Status label
    status_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(status_label_, 350);
    lv_label_set_text(status_label_, Strings::getInstance().get("backup.choose_provider"));
    lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);

    // Provider list
    provider_list_ = lv_list_create(screen_);
    lv_obj_set_size(provider_list_, 350, 250);
    lv_obj_align(provider_list_, LV_ALIGN_TOP_MID, 0, 170);

    // GitHub repository option
    lv_obj_t* btn = lv_list_add_button(provider_list_, LV_SYMBOL_UPLOAD,
        Strings::getInstance().get("backup.github"));
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenBackup* screen = (ScreenBackup*)lv_obj_get_user_data(target);
        if (screen && screen->configure_cb_) {
            screen->configure_cb_(BackupProvider::GITHUB_REPO);
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(btn, this);

    // GitHub Gist option
    btn = lv_list_add_button(provider_list_, LV_SYMBOL_FILE,
        Strings::getInstance().get("backup.gist"));
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenBackup* screen = (ScreenBackup*)lv_obj_get_user_data(target);
        if (screen && screen->configure_cb_) {
            screen->configure_cb_(BackupProvider::GITHUB_GIST);
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(btn, this);

    // Back button
    back_btn_ = lv_button_create(screen_);
    lv_obj_set_size(back_btn_, 120, 50);
    lv_obj_align(back_btn_, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t* label = lv_label_create(back_btn_);
    lv_label_set_text(label, Strings::getInstance().get("common.back"));
    lv_obj_center(label);

    lv_obj_add_event_cb(back_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        ScreenBackup* screen = (ScreenBackup*)lv_obj_get_user_data(target);
        if (screen && screen->back_cb_) {
            screen->back_cb_();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(back_btn_, this);
}

void ScreenBackup::show() {
    if (screen_) {
        lv_screen_load(screen_);
        updateUI();
    }
}

void ScreenBackup::hide() {
    if (back_cb_) {
        back_cb_();
    }
}

void ScreenBackup::updateStatus(bool has_token, const std::string& repo_info) {
    has_token_ = has_token;
    repo_info_ = repo_info;
    updateUI();
}

void ScreenBackup::updateUI() {
    if (!status_label_) return;

    Strings& strings = Strings::getInstance();
    if (has_token_ && !repo_info_.empty()) {
        std::string text = strings.format("backup.configured", {{"info", repo_info_}});
        text += "\n\n";
        text += strings.get("backup.configured_hint");
        lv_label_set_text(status_label_, text.c_str());
    } else if (has_token_) {
        lv_label_set_text(status_label_, strings.get("backup.choose_provider"));
    } else {
        lv_label_set_text(status_label_, strings.get("backup.choose_provider"));
    }
}

// ============================================================================
// TokenInputDialog Implementation
// ============================================================================

TokenInputDialog& TokenInputDialog::getInstance() {
    static TokenInputDialog instance;
    return instance;
}

TokenInputDialog::~TokenInputDialog() {
    if (dialog_) {
        lv_obj_delete(dialog_);
    }
}

void TokenInputDialog::init() {
    // Dialog created on demand
}

void TokenInputDialog::createDialog() {
    if (dialog_) return;

    // Create modal dialog
    dialog_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(dialog_, LV_HOR_RES - 60, 280);
    lv_obj_center(dialog_);
    lv_obj_set_style_bg_opa(dialog_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dialog_, lv_color_white(), 0);
    lv_obj_set_style_border_width(dialog_, 2, 0);
    lv_obj_set_style_border_color(dialog_, lv_color_black(), 0);

    // Title
    lv_obj_t* title = lv_label_create(dialog_);
    lv_label_set_text(title, Strings::getInstance().get("backup.title"));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

    // Instructions
    lv_obj_t* info = lv_label_create(dialog_);
    lv_label_set_long_mode(info, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(info, LV_HOR_RES - 100);
    Strings& strings = Strings::getInstance();
    std::string info_text = std::string(strings.get("backup.token_prompt")) + "\n" +
                            strings.get("backup.token_hint");
    lv_label_set_text(info, info_text.c_str());
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, 0);

    // Token input
    textarea_ = lv_textarea_create(dialog_);
    lv_obj_set_size(textarea_, LV_HOR_RES - 100, 60);
    lv_obj_align(textarea_, LV_ALIGN_TOP_MID, 0, 110);
    lv_textarea_set_placeholder_text(textarea_, Strings::getInstance().get("backup.token_placeholder"));
    lv_textarea_set_password_mode(textarea_, true);
    lv_textarea_set_one_line(textarea_, true);
    lv_obj_add_flag(textarea_, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    // Buttons container
    lv_obj_t* btn_cont = lv_obj_create(dialog_);
    lv_obj_set_size(btn_cont, 250, 50);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);

    // Cancel button
    cancel_btn_ = lv_button_create(btn_cont);
    lv_obj_set_size(cancel_btn_, 100, 40);
    lv_obj_align(cancel_btn_, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* label = lv_label_create(cancel_btn_);
    lv_label_set_text(label, Strings::getInstance().get("common.cancel"));
    lv_obj_center(label);

    lv_obj_add_event_cb(cancel_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        TokenInputDialog* dlg = (TokenInputDialog*)lv_obj_get_user_data(target);
        if (dlg) {
            dlg->hide();
            if (dlg->cancel_callback_) {
                dlg->cancel_callback_();
            }
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(cancel_btn_, this);

    // Save button
    save_btn_ = lv_button_create(btn_cont);
    lv_obj_set_size(save_btn_, 100, 40);
    lv_obj_align(save_btn_, LV_ALIGN_RIGHT_MID, 0, 0);

    label = lv_label_create(save_btn_);
    lv_label_set_text(label, Strings::getInstance().get("common.save"));
    lv_obj_center(label);

    lv_obj_add_event_cb(save_btn_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        TokenInputDialog* dlg = (TokenInputDialog*)lv_obj_get_user_data(target);
        if (dlg) {
            dlg->validateToken();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(save_btn_, this);
}

void TokenInputDialog::show(TokenCallback on_save, CancelCallback on_cancel) {
    save_callback_ = on_save;
    cancel_callback_ = on_cancel;

    createDialog();

    if (textarea_) {
        lv_textarea_set_text(textarea_, "");
        lv_obj_add_state(textarea_, LV_STATE_FOCUSED);
    }

    lv_obj_clear_flag(dialog_, LV_OBJ_FLAG_HIDDEN);
}

void TokenInputDialog::hide() {
    if (dialog_) {
        lv_obj_add_flag(dialog_, LV_OBJ_FLAG_HIDDEN);
    }
}

void TokenInputDialog::validateToken() {
    if (!textarea_) return;

    const char* token = lv_textarea_get_text(textarea_);

    // Basic validation: tokens should be non-trivial length
    size_t len = strlen(token);
    if (len < 20) {
        // Show error
        lv_textarea_set_placeholder_text(textarea_, Strings::getInstance().get("backup.token_invalid"));
        return;
    }

    // Save token
    if (save_callback_) {
        save_callback_(std::string(token));
    }

    hide();
}

// ============================================================================
// RepoConfigDialog Implementation
// ============================================================================

RepoConfigDialog& RepoConfigDialog::getInstance() {
    static RepoConfigDialog instance;
    return instance;
}

RepoConfigDialog::~RepoConfigDialog() {
    if (dialog_) {
        lv_obj_delete(dialog_);
    }
}

void RepoConfigDialog::init() {
    // Dialog created on demand
}

void RepoConfigDialog::createDialog() {
    if (dialog_) return;

    // Create modal dialog
    dialog_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(dialog_, LV_HOR_RES - 60, 300);
    lv_obj_center(dialog_);
    lv_obj_set_style_bg_opa(dialog_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dialog_, lv_color_white(), 0);
    lv_obj_set_style_border_width(dialog_, 2, 0);
    lv_obj_set_style_border_color(dialog_, lv_color_black(), 0);

    // Title
    lv_obj_t* title = lv_label_create(dialog_);
    lv_label_set_text(title, Strings::getInstance().get("backup.repo_title"));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

    // Owner input
    lv_obj_t* label = lv_label_create(dialog_);
    lv_label_set_text(label, Strings::getInstance().get("backup.repo_owner"));
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 30, 60);

    owner_input_ = lv_textarea_create(dialog_);
    lv_obj_set_size(owner_input_, 200, 40);
    lv_obj_align(owner_input_, LV_ALIGN_TOP_LEFT, 30, 80);
    lv_textarea_set_placeholder_text(owner_input_, Strings::getInstance().get("backup.repo_owner_placeholder"));
    lv_textarea_set_one_line(owner_input_, true);

    // Repo input
    label = lv_label_create(dialog_);
    lv_label_set_text(label, Strings::getInstance().get("backup.repo_name"));
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 30, 130);

    repo_input_ = lv_textarea_create(dialog_);
    lv_obj_set_size(repo_input_, 200, 40);
    lv_obj_align(repo_input_, LV_ALIGN_TOP_LEFT, 30, 150);
    lv_textarea_set_placeholder_text(repo_input_, Strings::getInstance().get("backup.repo_name_placeholder"));
    lv_textarea_set_one_line(repo_input_, true);

    // Branch input
    label = lv_label_create(dialog_);
    lv_label_set_text(label, Strings::getInstance().get("backup.repo_branch"));
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 30, 200);

    branch_input_ = lv_textarea_create(dialog_);
    lv_obj_set_size(branch_input_, 200, 40);
    lv_obj_align(branch_input_, LV_ALIGN_TOP_LEFT, 30, 220);
    lv_textarea_set_placeholder_text(branch_input_, Strings::getInstance().get("backup.repo_branch_placeholder"));
    lv_textarea_set_one_line(branch_input_, true);
    lv_textarea_set_text(branch_input_, "main");

    // Buttons container
    lv_obj_t* btn_cont = lv_obj_create(dialog_);
    lv_obj_set_size(btn_cont, 250, 50);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);

    // Cancel button
    lv_obj_t* cancel_btn = lv_button_create(btn_cont);
    lv_obj_set_size(cancel_btn, 100, 40);
    lv_obj_align(cancel_btn, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* btn_label = lv_label_create(cancel_btn);
    lv_label_set_text(btn_label, Strings::getInstance().get("common.cancel"));
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(cancel_btn, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        RepoConfigDialog* dlg = (RepoConfigDialog*)lv_obj_get_user_data(target);
        if (dlg) {
            dlg->hide();
            if (dlg->cancel_callback_) {
                dlg->cancel_callback_();
            }
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(cancel_btn, this);

    // Save button
    lv_obj_t* save_btn = lv_button_create(btn_cont);
    lv_obj_set_size(save_btn, 100, 40);
    lv_obj_align(save_btn, LV_ALIGN_RIGHT_MID, 0, 0);

    btn_label = lv_label_create(save_btn);
    lv_label_set_text(btn_label, Strings::getInstance().get("backup.repo_save"));
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(save_btn, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        RepoConfigDialog* dlg = (RepoConfigDialog*)lv_obj_get_user_data(target);
        if (dlg && dlg->save_callback_) {
            const char* owner = lv_textarea_get_text(dlg->owner_input_);
            const char* repo = lv_textarea_get_text(dlg->repo_input_);
            const char* branch = lv_textarea_get_text(dlg->branch_input_);
            dlg->save_callback_(std::string(owner), std::string(repo), std::string(branch));
        }
        if (dlg) {
            dlg->hide();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(save_btn, this);
}

void RepoConfigDialog::show(ConfigCallback on_save, CancelCallback on_cancel) {
    save_callback_ = on_save;
    cancel_callback_ = on_cancel;

    createDialog();

    lv_obj_clear_flag(dialog_, LV_OBJ_FLAG_HIDDEN);
}

void RepoConfigDialog::hide() {
    if (dialog_) {
        lv_obj_add_flag(dialog_, LV_OBJ_FLAG_HIDDEN);
    }
}
