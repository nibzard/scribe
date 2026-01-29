#include "dialog_power_off.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_DIALOG_POWER_OFF";

DialogPowerOff::DialogPowerOff() : dialog_(nullptr) {
}

DialogPowerOff::~DialogPowerOff() {
    if (dialog_) {
        lv_obj_delete(dialog_);
    }
}

void DialogPowerOff::init() {
    ESP_LOGI(TAG, "Initializing power off dialog");
    createWidgets();
}

void DialogPowerOff::createWidgets() {
    // Create dialog as a modal overlay
    dialog_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(dialog_, Theme::fitWidth(300, 40), Theme::fitHeight(150, 200));
    lv_obj_center(dialog_);
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(dialog_, colors.fg, 0);
    lv_obj_set_style_border_color(dialog_, colors.border, 0);
    lv_obj_set_style_border_width(dialog_, 2, 0);
    lv_obj_set_style_text_font(dialog_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
    lv_obj_add_flag(dialog_, LV_OBJ_FLAG_HIDDEN);

    // Question
    question_label_ = lv_label_create(dialog_);
    lv_label_set_text(question_label_, Strings::getInstance().get("power.off_confirm"));
    lv_obj_align(question_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(20));
    lv_obj_set_style_text_font(question_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Yes button
    btn_yes_ = lv_button_create(dialog_);
    lv_obj_set_size(btn_yes_, Theme::scalePx(100), Theme::scalePx(40));
    lv_obj_align(btn_yes_, LV_ALIGN_CENTER, -Theme::scalePx(60), Theme::scalePx(30));
    lv_obj_set_style_text_font(btn_yes_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    lv_obj_t* label = lv_label_create(btn_yes_);
    lv_label_set_text(label, Strings::getInstance().get("power.off_yes"));
    lv_obj_center(label);

    lv_obj_add_event_cb(btn_yes_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        DialogPowerOff* dialog = (DialogPowerOff*)lv_obj_get_user_data(lv_obj_get_parent(target));
        if (dialog && dialog->confirm_cb_) {
            dialog->confirm_cb_();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(dialog_, this);

    // No button
    btn_no_ = lv_button_create(dialog_);
    lv_obj_set_size(btn_no_, Theme::scalePx(100), Theme::scalePx(40));
    lv_obj_align(btn_no_, LV_ALIGN_CENTER, Theme::scalePx(60), Theme::scalePx(30));
    lv_obj_set_style_text_font(btn_no_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    label = lv_label_create(btn_no_);
    lv_label_set_text(label, Strings::getInstance().get("power.off_no"));
    lv_obj_center(label);

    lv_obj_add_event_cb(btn_no_, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        DialogPowerOff* dialog = (DialogPowerOff*)lv_obj_get_user_data(lv_obj_get_parent(target));
        if (dialog && dialog->cancel_cb_) {
            dialog->cancel_cb_();
        }
    }, LV_EVENT_CLICKED, nullptr);
}

void DialogPowerOff::show() {
    if (dialog_) {
        const Theme::Colors& colors = Theme::getColors();
        lv_obj_set_style_bg_color(dialog_, colors.fg, 0);
        lv_obj_set_style_border_color(dialog_, colors.border, 0);
        lv_obj_set_size(dialog_, Theme::fitWidth(300, 40), Theme::fitHeight(150, 200));
        lv_obj_center(dialog_);
        lv_obj_set_style_text_font(dialog_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        if (question_label_) {
            lv_obj_set_style_text_color(question_label_, colors.text, 0);
            lv_obj_set_style_text_font(question_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
            lv_obj_align(question_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(20));
        }
        if (btn_yes_) {
            lv_obj_set_size(btn_yes_, Theme::scalePx(100), Theme::scalePx(40));
            lv_obj_align(btn_yes_, LV_ALIGN_CENTER, -Theme::scalePx(60), Theme::scalePx(30));
            lv_obj_set_style_text_font(btn_yes_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (btn_no_) {
            lv_obj_set_size(btn_no_, Theme::scalePx(100), Theme::scalePx(40));
            lv_obj_align(btn_no_, LV_ALIGN_CENTER, Theme::scalePx(60), Theme::scalePx(30));
            lv_obj_set_style_text_font(btn_no_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        lv_obj_clear_flag(dialog_, LV_OBJ_FLAG_HIDDEN);
    }
}

void DialogPowerOff::hide() {
    if (dialog_) {
        lv_obj_add_flag(dialog_, LV_OBJ_FLAG_HIDDEN);
    }
}
