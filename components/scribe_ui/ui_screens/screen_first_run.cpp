#include "screen_first_run.h"
#include "../../scribe_utils/strings.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_FIRST_RUN";

ScreenFirstRun::ScreenFirstRun() : screen_(nullptr) {
}

ScreenFirstRun::~ScreenFirstRun() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenFirstRun::init() {
    ESP_LOGI(TAG, "Initializing first run screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenFirstRun::createWidgets() {
    // Welcome title
    lv_obj_t* title = lv_label_create(screen_);
    lv_label_set_text(title, Strings::getInstance().get("app.name"));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    // Tagline
    lv_obj_t* tagline = lv_label_create(screen_);
    lv_label_set_text(tagline, Strings::getInstance().get("boot.tagline"));
    lv_obj_align(tagline, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_text_font(tagline, &lv_font_montserrat_14, 0);

    // Main tip in a scrollable area
    lv_obj_t* content = lv_obj_create(screen_);
    lv_obj_set_size(content, LV_HOR_RES - 60, 250);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 120);

    lv_obj_t* label = lv_label_create(content);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - 80);
    lv_label_set_text(label, Strings::getInstance().get("first_run.tip"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label);

    // Dismiss instruction
    label = lv_label_create(screen_);
    lv_label_set_text(label, Strings::getInstance().get("first_run.dismiss"));
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -30);
}

void ScreenFirstRun::show() {
    if (screen_) {
        lv_screen_load(screen_);
    }
}

void ScreenFirstRun::hide() {
    if (dismiss_cb_) {
        dismiss_cb_();
    }
}
