#include "screen_ai_prompt.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_AI_PROMPT";

ScreenAIPrompt::ScreenAIPrompt() : dialog_(nullptr) {
}

ScreenAIPrompt::~ScreenAIPrompt() {
    if (dialog_) {
        lv_obj_delete(dialog_);
    }
}

void ScreenAIPrompt::init() {
    ESP_LOGI(TAG, "Initializing AI prompt dialog");
    createWidgets();
}

void ScreenAIPrompt::createWidgets() {
    dialog_ = lv_obj_create(lv_layer_top());

    int width = LV_HOR_RES - Theme::scalePx(60);
    int max_width = Theme::scalePx(520);
    if (width > max_width) {
        width = max_width;
    }

    int height = Theme::scalePx(180);
    int max_height = LV_VER_RES - Theme::scalePx(80);
    if (height > max_height) {
        height = max_height;
    }

    lv_obj_set_size(dialog_, width, height);
    lv_obj_center(dialog_);

    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(dialog_, colors.fg, 0);
    lv_obj_set_style_bg_opa(dialog_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dialog_, colors.border, 0);
    lv_obj_set_style_border_width(dialog_, 2, 0);
    lv_obj_set_style_radius(dialog_, 12, 0);
    lv_obj_set_style_pad_all(dialog_, Theme::scalePx(12), 0);
    lv_obj_add_flag(dialog_, LV_OBJ_FLAG_HIDDEN);

    title_label_ = lv_label_create(dialog_);
    lv_label_set_text(title_label_, Strings::getInstance().get("ai.prompt_title"));
    lv_obj_align(title_label_, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
    lv_obj_set_style_text_color(title_label_, colors.text, 0);

    prompt_input_ = lv_textarea_create(dialog_);
    lv_obj_set_size(prompt_input_, width - Theme::scalePx(24), Theme::scalePx(48));
    lv_obj_align(prompt_input_, LV_ALIGN_TOP_LEFT, 0, Theme::scalePx(40));
    lv_textarea_set_placeholder_text(prompt_input_, Strings::getInstance().get("ai.prompt_placeholder"));
    lv_textarea_set_one_line(prompt_input_, true);
    lv_obj_add_flag(prompt_input_, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_set_style_bg_color(prompt_input_, colors.bg, 0);
    lv_obj_set_style_bg_opa(prompt_input_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(prompt_input_, colors.border, 0);
    lv_obj_set_style_border_width(prompt_input_, 1, 0);
    lv_obj_set_style_text_color(prompt_input_, colors.text, 0);
    lv_obj_set_style_text_color(prompt_input_, colors.text_secondary, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_font(prompt_input_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    hint_label_ = lv_label_create(dialog_);
    lv_label_set_text(hint_label_, Strings::getInstance().get("ai.prompt_hint"));
    lv_obj_align(hint_label_, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(hint_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
    lv_obj_set_style_text_color(hint_label_, colors.text_secondary, 0);
}

void ScreenAIPrompt::show() {
    if (!dialog_) {
        return;
    }

    const Theme::Colors& colors = Theme::getColors();
    int width = LV_HOR_RES - Theme::scalePx(60);
    int max_width = Theme::scalePx(520);
    if (width > max_width) {
        width = max_width;
    }

    int height = Theme::scalePx(180);
    int max_height = LV_VER_RES - Theme::scalePx(80);
    if (height > max_height) {
        height = max_height;
    }

    lv_obj_set_size(dialog_, width, height);
    lv_obj_center(dialog_);

    lv_obj_set_style_bg_color(dialog_, colors.fg, 0);
    lv_obj_set_style_border_color(dialog_, colors.border, 0);

    if (title_label_) {
        lv_obj_set_style_text_color(title_label_, colors.text, 0);
        lv_obj_set_style_text_font(title_label_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
        lv_obj_align(title_label_, LV_ALIGN_TOP_LEFT, 0, 0);
    }

    if (prompt_input_) {
        lv_obj_set_size(prompt_input_, width - Theme::scalePx(24), Theme::scalePx(48));
        lv_obj_align(prompt_input_, LV_ALIGN_TOP_LEFT, 0, Theme::scalePx(40));
        lv_obj_set_style_bg_color(prompt_input_, colors.bg, 0);
        lv_obj_set_style_border_color(prompt_input_, colors.border, 0);
        lv_obj_set_style_text_color(prompt_input_, colors.text, 0);
        lv_obj_set_style_text_color(prompt_input_, colors.text_secondary, LV_PART_TEXTAREA_PLACEHOLDER);
        lv_obj_set_style_text_font(prompt_input_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        lv_textarea_set_text(prompt_input_, "");
        lv_obj_add_state(prompt_input_, LV_STATE_FOCUSED);
    }

    if (hint_label_) {
        lv_obj_set_style_text_color(hint_label_, colors.text_secondary, 0);
        lv_obj_set_style_text_font(hint_label_, Theme::getUIFont(Theme::UiFontRole::Small), 0);
        lv_obj_align(hint_label_, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }

    lv_obj_clear_flag(dialog_, LV_OBJ_FLAG_HIDDEN);
    visible_ = true;
}

void ScreenAIPrompt::hide() {
    if (dialog_) {
        lv_obj_add_flag(dialog_, LV_OBJ_FLAG_HIDDEN);
    }
    visible_ = false;
}

void ScreenAIPrompt::clearPrompt() {
    if (prompt_input_) {
        lv_textarea_set_text(prompt_input_, "");
    }
}

void ScreenAIPrompt::appendPromptChar(char c) {
    if (!prompt_input_) {
        return;
    }
    char buf[2] = {c, 0};
    lv_textarea_add_text(prompt_input_, buf);
}

void ScreenAIPrompt::backspacePrompt() {
    if (prompt_input_) {
        lv_textarea_delete_char(prompt_input_);
    }
}

std::string ScreenAIPrompt::getPrompt() const {
    if (prompt_input_) {
        const char* text = lv_textarea_get_text(prompt_input_);
        return std::string(text ? text : "");
    }
    return "";
}
