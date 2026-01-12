#include "screen_help.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_HELP";

ScreenHelp::ScreenHelp() : screen_(nullptr) {
}

ScreenHelp::~ScreenHelp() {
    if (screen_) {
        lv_obj_del(screen_);
    }
}

void ScreenHelp::init() {
    ESP_LOGI(TAG, "Initializing help screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenHelp::createWidgets() {
    // Title
    lv_obj_t* title = lv_label_create(screen_);
    lv_label_set_text(title, "Help");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    // Help content in a scrollable area
    lv_obj_t* content = lv_obj_create(screen_);
    lv_obj_set_size(content, LV_HOR_RES - 40, LV_VER_RES - 100);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    // Section: How to write
    lv_obj_t* label = lv_label_create(content);
    lv_label_set_text(label, "How to write");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

    label = lv_label_create(content);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - 60);
    lv_label_set_text(label, "Just start typing! The screen is your page.");
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Section: Shortcuts
    label = lv_label_create(content);
    lv_label_set_text(label, "\nShortcuts");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

    label = lv_label_create(content);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - 60);
    lv_label_set_text(label,
        "Esc - Menu\n"
        "F1 / Hold Space - Status\n"
        "Ctrl+S - Save\n"
        "Ctrl+F - Find\n"
        "Ctrl+E - Export\n"
        "Ctrl+N - New project\n"
        "Ctrl+O - Switch project\n"
        "Ctrl+M - Toggle mode\n"
        "Ctrl+K - AI Magic Bar\n"
        "Ctrl+Q - Power off\n"
        "Ctrl+Z/Y - Undo/Redo"
    );
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Section: Saving
    label = lv_label_create(content);
    lv_label_set_text(label, "\nHow saving works");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

    label = lv_label_create(content);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - 60);
    lv_label_set_text(label, "Your work is saved automatically. Press Ctrl+S to save manually and create snapshots.");
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Section: Export
    label = lv_label_create(content);
    lv_label_set_text(label, "\nHow to export");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

    label = lv_label_create(content);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_HOR_RES - 60);
    lv_label_set_text(label, "Use Send to Computer to type your draft into any app on your computer. Or export to SD card.");
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    // Close instruction
    label = lv_label_create(screen_);
    lv_label_set_text(label, "Press Esc to close");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void ScreenHelp::show() {
    if (screen_) {
        lv_scr_load(screen_);
    }
}

void ScreenHelp::hide() {
    if (close_cb_) {
        close_cb_();
    }
}
