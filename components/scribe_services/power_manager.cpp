#include "power_manager.h"
#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include "battery.h"
#include "../scribe_hw/tab5_io_expander.h"
#include "../scribe_ui/mipi_dsi_display.h"

static const char* TAG = "SCRIBE_POWER";

// Hardware configuration
#define WAKE_GPIO_MASK          (1ULL << 0)  // GPIO0 as wake source

PowerManager& PowerManager::getInstance() {
    static PowerManager instance;
    return instance;
}

esp_err_t PowerManager::init() {
    ESP_LOGI(TAG, "Initializing power management...");

    last_activity_time_ = xTaskGetTickCount();

    updateBatteryStatus();

    return ESP_OK;
}

void PowerManager::updateBatteryStatus() {
    Battery& battery = Battery::getInstance();
    battery.update();

    battery_percentage_.store(battery.getPercentage());
    charging_.store(battery.isCharging());

    if (battery_callback_) {
        battery_callback_(battery_percentage_.load(), charging_.load());
    }

    ESP_LOGD(TAG, "Battery: %d%%, Charging: %s", battery_percentage_.load(),
             charging_.load() ? "yes" : "no");
}

esp_err_t PowerManager::configureWakeSources() {
    // Configure GPIO0 as wake source using EXT1 (EXT0 not available on esp32p4)
    esp_sleep_enable_ext1_wakeup(1ULL << GPIO_NUM_0, ESP_EXT1_WAKEUP_ANY_LOW);

    // Enable touch pad wake (for capacitive buttons)
    // esp_sleep_enable_touchpad_wakeup();

    return ESP_OK;
}

esp_err_t PowerManager::prepareForSleep() {
    ESP_LOGI(TAG, "Preparing for sleep...");

    // Turn off display and backlight
    MIPIDSI::setBacklight(0);
    MIPIDSI::sleep(true);

    // TODO: Stop USB host (save power)
    // TODO: Reduce CPU frequency

    // Configure wake sources
    configureWakeSources();

    return ESP_OK;
}

esp_err_t PowerManager::resumeFromSleep() {
    ESP_LOGI(TAG, "Resuming from sleep...");

    // Wake display and restore backlight
    MIPIDSI::sleep(false);
    MIPIDSI::setBacklight(50);  // Default brightness on wake

    // TODO: Restart USB host
    // TODO: Restore CPU frequency

    return ESP_OK;
}

esp_err_t PowerManager::enterSleep() {
    if (state_.load() == PowerState::SLEEPING) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Entering sleep mode...");
    state_.store(PowerState::SLEEPING);

    prepareForSleep();

    // Use light sleep for faster wake
    esp_light_sleep_start();

    return wake();
}

esp_err_t PowerManager::wake() {
    if (state_.load() != PowerState::SLEEPING) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Waking from sleep...");

    resumeFromSleep();
    state_.store(PowerState::ACTIVE);
    resetActivityTimer();

    // Call wake callback
    if (wake_callback_) {
        wake_callback_();
    }

    return ESP_OK;
}

esp_err_t PowerManager::requestPowerOff() {
    ESP_LOGI(TAG, "Power off requested - confirmation should be shown by UI");
    // UI should show confirmation dialog before calling powerOff()
    return ESP_OK;
}

esp_err_t PowerManager::powerOff() {
    ESP_LOGI(TAG, "Shutting down...");

    // TODO: Ensure all file handles are closed
    // TODO: Unmount SD card
    // TODO: Turn off all peripherals

    tab5::IOExpander::getInstance().init();
    tab5::IOExpander::getInstance().pulsePowerOff();

    // Enter deep sleep (effectively power off)
    esp_deep_sleep_start();

    return ESP_OK;  // Never reached
}

void PowerManager::resetActivityTimer() {
    last_activity_time_ = xTaskGetTickCount();
}

void PowerManager::activityTimerCallback(TimerHandle_t timer) {
    PowerManager* pm = static_cast<PowerManager*>(pvTimerGetTimerID(timer));
    pm->checkInactivitySleep();
}

void PowerManager::checkInactivitySleep() {
    if (!activity_monitoring_.load() || state_.load() != PowerState::ACTIVE) {
        return;
    }

    if (auto_sleep_timeout_ == AutoSleepTimeout::OFF) {
        return;
    }

    // Calculate timeout in ticks
    TickType_t timeout_ms = 0;
    switch (auto_sleep_timeout_) {
        case AutoSleepTimeout::MIN_5:
            timeout_ms = 5 * 60 * 1000;
            break;
        case AutoSleepTimeout::MIN_15:
            timeout_ms = 15 * 60 * 1000;
            break;
        case AutoSleepTimeout::MIN_30:
            timeout_ms = 30 * 60 * 1000;
            break;
        default:
            return;
    }

    TickType_t elapsed_ms = (xTaskGetTickCount() - last_activity_time_) * portTICK_PERIOD_MS;

    if (elapsed_ms >= timeout_ms) {
        ESP_LOGI(TAG, "Inactivity timeout - entering sleep");
        enterSleep();
    }
}

esp_err_t PowerManager::startActivityMonitoring() {
    if (activity_monitoring_.load()) {
        return ESP_OK;
    }

    if (!activity_timer_) {
        activity_timer_ = xTimerCreate(
            "activity_timer",
            pdMS_TO_TICKS(60000),  // Check every minute
            pdTRUE,                 // Auto-reload
            this,
            activityTimerCallback
        );

        if (!activity_timer_) {
            ESP_LOGE(TAG, "Failed to create activity timer");
            return ESP_ERR_NO_MEM;
        }
    }

    if (xTimerStart(activity_timer_, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to start activity timer");
        return ESP_FAIL;
    }

    activity_monitoring_.store(true);
    ESP_LOGI(TAG, "Activity monitoring started");
    return ESP_OK;
}

esp_err_t PowerManager::stopActivityMonitoring() {
    if (!activity_monitoring_.load()) {
        return ESP_OK;
    }

    if (activity_timer_) {
        xTimerStop(activity_timer_, 0);
    }

    activity_monitoring_.store(false);
    ESP_LOGI(TAG, "Activity monitoring stopped");
    return ESP_OK;
}
