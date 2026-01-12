#include "screen_backup.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_BACKUP";

ScreenBackup::ScreenBackup() : screen_(nullptr) {
}

ScreenBackup::~ScreenBackup() {
    if (screen_) {
        lv_obj_del(screen_);
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
    lv_label_set_text(title_label_, "Cloud backup");
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_20, 0);

    // Description
    description_label_ = lv_label_create(screen_);
    lv_label_set_long_mode(description_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(description_label_, 350);
    lv_label_set_text(description_label_, "Your writing is always saved locally. Cloud backup is optional.");
    lv_obj_align(description_label_, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_text_align(description_label_, LV_TEXT_ALIGN_CENTER, 0);

    // Status label
    status_label_ = lv_label_create(screen_);
    lv_label_set_text(status_label_, "Choose a backup");
    lv_obj_align(status_label_, LV_ALIGN_TOP_MID, 0, 130);

    // Provider options
    provider_list_ = lv_list_create(screen_);
    lv_obj_set_size(provider_list_, 350, 200);
    lv_obj_align(provider_list_, LV_ALIGN_TOP_MID, 0, 170);

    // GitHub repository option
    lv_obj_t* btn = lv_list_add_btn(provider_list_, LV_SYMBOL_GITHUB, "GitHub repository");
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

    // GitHub Gist option
    btn = lv_list_add_btn(provider_list_, LV_SYMBOL_FILE, "GitHub Gist");
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

    // Back button
    btn = lv_btn_create(screen_);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, "Back");
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, [](lv_obj_t* obj, lv_event_t* event) {
        ScreenBackup* screen = (ScreenBackup*)lv_obj_get_user_data(obj);
        if (screen && screen->back_cb_) {
            screen->back_cb_();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(btn, this);
}

void ScreenBackup::show() {
    if (screen_) {
        lv_scr_load(screen_);
        updateStatus();
    }
}

void ScreenBackup::hide() {
    if (back_cb_) {
        back_cb_();
    }
}

void ScreenBackup::updateStatus() {
    // TODO: Check if backup is configured and update status
    ESP_LOGI(TAG, "Backup settings shown");
}
