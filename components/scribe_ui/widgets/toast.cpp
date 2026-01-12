#include "toast.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_TOAST";

Toast::Toast(lv_obj_t* parent) : lv_obj() {
    // Create LVGL object
    // TODO: Implement custom LVGL widget
}

void Toast::show(const char* message, uint32_t duration_ms) {
    if (label_) {
        lv_label_set_text(label_, message);
        lv_obj_clear_flag(this, LV_OBJ_FLAG_HIDDEN);
    }

    // Set hide timer
    if (hide_timer_) {
        lv_timer_reset(hide_timer_);
    } else {
        hide_timer_ = lv_timer_create([](lv_timer_t* t) {
            Toast* toast = (Toast*)t->user_data;
            toast->hide();
        }, duration_ms, this);
    }
}

void Toast::hide() {
    lv_obj_add_flag(this, LV_OBJ_FLAG_HIDDEN);
}

void Toast::onHideTimer(lv_timer_t* timer) {
    hide();
}
