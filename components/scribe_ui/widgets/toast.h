#pragma once

#include <lvgl.h>
#include <string>
#include <functional>

// Toast notification - temporary overlay messages
// "Saved ✓", "AI is off", etc.

class Toast {
public:
    using HideCallback = std::function<void()>;

    Toast(lv_obj_t* parent);
    ~Toast();

    // Show toast message (auto-hide after delay)
    void show(const char* message, uint32_t duration_ms = 2000);
    void show(const std::string& message, uint32_t duration_ms = 2000);

    // Hide immediately
    void hide();

    // Set callback for when toast is hidden
    void setHideCallback(HideCallback cb) { hide_callback_ = cb; }

    bool isVisible() const { return visible_; }

private:
    bool visible_ = false;
    lv_obj_t* container_ = nullptr;
    lv_obj_t* label_ = nullptr;
    lv_timer_t* hide_timer_ = nullptr;
    HideCallback hide_callback_;

    void createWidgets();
    void startHideTimer(uint32_t duration_ms);
};
