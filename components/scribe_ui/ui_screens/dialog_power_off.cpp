#include "dialog_power_off.h"
#include "../../scribe_utils/strings.h"
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
    lv_obj_set_size(dialog_, 300, 150);
    lv_obj_center(dialog_);
    lv_obj_set_style_bg_color(dialog_, lv_color_white(), 0);
    lv_obj_set_style_border_color(dialog_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(dialog_, 2, 0);
    lv_obj_add_flag(dialog_, LV_OBJ_FLAG_HIDDEN);

    // Question
    lv_obj_t* label = lv_label_create(dialog_);
    lv_label_set_text(label, Strings::getInstance().get("power.off_confirm"));
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

    // Yes button
    lv_obj_t* btn_yes = lv_button_create(dialog_);
    lv_obj_set_size(btn_yes, 100, 40);
    lv_obj_align(btn_yes, LV_ALIGN_CENTER, -60, 30);

    label = lv_label_create(btn_yes);
    lv_label_set_text(label, Strings::getInstance().get("power.off_yes"));
    lv_obj_center(label);

    lv_obj_add_event_cb(btn_yes, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        DialogPowerOff* dialog = (DialogPowerOff*)lv_obj_get_user_data(lv_obj_get_parent(target));
        if (dialog && dialog->confirm_cb_) {
            dialog->confirm_cb_();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_user_data(dialog_, this);

    // No button
    lv_obj_t* btn_no = lv_button_create(dialog_);
    lv_obj_set_size(btn_no, 100, 40);
    lv_obj_align(btn_no, LV_ALIGN_CENTER, 60, 30);

    label = lv_label_create(btn_no);
    lv_label_set_text(label, Strings::getInstance().get("power.off_no"));
    lv_obj_center(label);

    lv_obj_add_event_cb(btn_no, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_target_obj(e);
        DialogPowerOff* dialog = (DialogPowerOff*)lv_obj_get_user_data(lv_obj_get_parent(target));
        if (dialog && dialog->cancel_cb_) {
            dialog->cancel_cb_();
        }
    }, LV_EVENT_CLICKED, nullptr);
}

void DialogPowerOff::show() {
    if (dialog_) {
        lv_obj_clear_flag(dialog_, LV_OBJ_FLAG_HIDDEN);
    }
}

void DialogPowerOff::hide() {
    if (dialog_) {
        lv_obj_add_flag(dialog_, LV_OBJ_FLAG_HIDDEN);
    }
}
