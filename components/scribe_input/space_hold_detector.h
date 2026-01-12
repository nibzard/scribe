#pragma once

#include "key_event.h"
#include <functional>
#include <esp_timer.h>

// Space hold detector - detects when Space is held for 1 second to toggle HUD
// Based on SPECS.md: "Hold Space for 1 second (recommended)" to invoke HUD

class SpaceHoldDetector {
public:
    using HUDCallback = std::function<void()>;
    using TapCallback = std::function<void()>;

    SpaceHoldDetector();
    ~SpaceHoldDetector();

    // Initialize detector
    void init();

    // Process key event - returns true if handled
    bool processEvent(const KeyEvent& event);

    // Set callback for HUD toggle
    void setHUDCallback(HUDCallback cb) { hud_cb_ = cb; }
    void setTapCallback(TapCallback cb) { tap_cb_ = cb; }

private:
    static const int HOLD_DURATION_MS = 1000;  // 1 second hold

    HUDCallback hud_cb_;
    TapCallback tap_cb_;
    bool space_pressed_ = false;
    bool hold_triggered_ = false;
    esp_timer_handle_t hold_timer_ = nullptr;

    // Timer callback
    static void holdTimerCallback(void* arg);

    // Start hold timer
    void startHoldTimer();

    // Stop hold timer
    void stopHoldTimer();
};
