#include "space_hold_detector.h"
#include <esp_log.h>
#include <esp_timer.h>

static const char* TAG = "SCRIBE_SPACE_HOLD";

SpaceHoldDetector::SpaceHoldDetector() : space_pressed_(false), hold_timer_(nullptr) {
}

SpaceHoldDetector::~SpaceHoldDetector() {
    if (hold_timer_) {
        esp_timer_delete(hold_timer_);
    }
}

void SpaceHoldDetector::init() {
    const esp_timer_create_args_t timer_args = {
        .callback = &holdTimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "space_hold"
    };

    esp_timer_create(&timer_args, &hold_timer_);
    ESP_LOGI(TAG, "Space hold detector initialized");
}

bool SpaceHoldDetector::processEvent(const KeyEvent& event) {
    if (event.key != KeyEvent::Key::SPACE) {
        return false;
    }

    if (event.pressed) {
        if (!space_pressed_) {
            space_pressed_ = true;
            hold_triggered_ = false;
            startHoldTimer();
        }
        return true;
    }

    if (space_pressed_) {
        space_pressed_ = false;
        stopHoldTimer();
        if (!hold_triggered_ && tap_cb_) {
            tap_cb_();
        }
        return true;
    }

    return false;
}

void SpaceHoldDetector::startHoldTimer() {
    if (hold_timer_) {
        esp_timer_start_once(hold_timer_, HOLD_DURATION_MS * 1000);
    }
}

void SpaceHoldDetector::stopHoldTimer() {
    if (hold_timer_) {
        esp_timer_stop(hold_timer_);
    }
}

void SpaceHoldDetector::holdTimerCallback(void* arg) {
    SpaceHoldDetector* detector = static_cast<SpaceHoldDetector*>(arg);

    if (detector->space_pressed_ && !detector->hold_triggered_) {
        ESP_LOGI(TAG, "Space held for 1 second - toggling HUD");
        detector->hold_triggered_ = true;
        detector->stopHoldTimer();

        if (detector->hud_cb_) {
            detector->hud_cb_();
        }
    }
}
