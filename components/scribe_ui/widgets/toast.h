#pragma once

#include <lvgl.h>
#include <stdint.h>

// Toast notification - temporary overlay messages
// "Saved ✓", "AI is off", etc.

class Toast : public lv_obj {
public:
    Toast(lv_obj_t* parent);

    // Show toast message (auto-hide after delay)
    void show(const char* message, uint32_t duration_ms = 2000);

    // Hide immediately
    void hide();

private:
    lv_obj_t* label_ = nullptr;
    lv_timer_t* hide_timer_ = nullptr;

    void onHideTimer(lv_timer_t* timer);
};
