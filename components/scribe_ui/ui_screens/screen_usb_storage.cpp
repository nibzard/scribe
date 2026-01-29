#include "screen_usb_storage.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_SCREEN_USB_STORAGE";

ScreenUsbStorage::ScreenUsbStorage() : screen_(nullptr) {
}

ScreenUsbStorage::~ScreenUsbStorage() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenUsbStorage::init() {
    ESP_LOGI(TAG, "Initializing USB storage screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenUsbStorage::createWidgets() {
    const Theme::Colors& colors = Theme::getColors();

    title_label_ = lv_label_create(screen_);
    lv_label_set_text(title_label_, Strings::getInstance().get("usb_storage.title"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(30));
    lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);

    content_ = lv_obj_create(screen_);
    lv_obj_set_size(content_, LV_HOR_RES - Theme::scalePx(60), Theme::fitHeight(260, 160));
    lv_obj_align(content_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(80));
    lv_obj_set_style_bg_color(content_, colors.fg, 0);
    lv_obj_set_style_bg_opa(content_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(content_, colors.border, 0);
    lv_obj_set_style_border_width(content_, 1, 0);
    lv_obj_set_style_text_font(content_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    body_label_ = lv_label_create(content_);
    lv_label_set_long_mode(body_label_, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(body_label_, LV_HOR_RES - Theme::scalePx(100));
    lv_label_set_text(body_label_, Strings::getInstance().get("usb_storage.body"));
    lv_obj_set_style_text_align(body_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(body_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(16));

    status_label_ = lv_label_create(content_);
    lv_label_set_text(status_label_, Strings::getInstance().get("usb_storage.status_disconnected"));
    lv_obj_set_style_text_font(status_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(status_label_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(16));

    hint_label_ = lv_label_create(screen_);
    lv_label_set_text(hint_label_, Strings::getInstance().get("usb_storage.hint"));
    lv_obj_set_style_text_font(hint_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
    lv_obj_align(hint_label_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(30));
}

void ScreenUsbStorage::show() {
    if (!screen_) {
        return;
    }
    Theme::applyScreenStyle(screen_);
    const Theme::Colors& colors = Theme::getColors();

    if (title_label_) {
        lv_obj_set_style_text_color(title_label_, colors.text, 0);
        lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
        lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(30));
    }
    if (content_) {
        lv_obj_set_size(content_, LV_HOR_RES - Theme::scalePx(60), Theme::fitHeight(260, 160));
        lv_obj_align(content_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(80));
        lv_obj_set_style_bg_color(content_, colors.fg, 0);
        lv_obj_set_style_border_color(content_, colors.border, 0);
        lv_obj_set_style_text_font(content_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
    }
    if (body_label_) {
        lv_obj_set_style_text_color(body_label_, colors.text, 0);
        lv_obj_set_style_text_font(body_label_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        lv_obj_set_width(body_label_, LV_HOR_RES - Theme::scalePx(100));
    }
    if (status_label_) {
        Strings& strings = Strings::getInstance();
        lv_label_set_text(status_label_, connected_ ? strings.get("usb_storage.status_connected")
                                                   : strings.get("usb_storage.status_disconnected"));
        lv_obj_set_style_text_color(status_label_, connected_ ? colors.success : colors.warning, 0);
        lv_obj_set_style_text_font(status_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
        lv_obj_align(status_label_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(16));
    }
    if (hint_label_) {
        lv_obj_set_style_text_color(hint_label_, colors.text_secondary, 0);
        lv_obj_set_style_text_font(hint_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
        lv_obj_align(hint_label_, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(30));
    }
    lv_screen_load(screen_);
}

void ScreenUsbStorage::hide() {
    if (back_cb_) {
        back_cb_();
    }
}

void ScreenUsbStorage::setConnected(bool connected) {
    connected_ = connected;
    if (!status_label_) {
        return;
    }
    Strings& strings = Strings::getInstance();
    lv_label_set_text(status_label_, connected ? strings.get("usb_storage.status_connected")
                                              : strings.get("usb_storage.status_disconnected"));
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_text_color(status_label_, connected ? colors.success : colors.warning, 0);
}
