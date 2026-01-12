#include "power_manager.h"
#include <esp_log.h>
#include <esp_sleep.h>

static const char* TAG = "SCRIBE_POWER";

PowerManager& PowerManager::getInstance() {
    static PowerManager instance;
    return instance;
}

esp_err_t PowerManager::init() {
    ESP_LOGI(TAG, "Initializing power management...");

    // TODO: Configure ADC for battery voltage monitoring
    // TODO: Set up GPIO for charging status detection

    updateBatteryStatus();

    return ESP_OK;
}

void PowerManager::updateBatteryStatus() {
    // TODO: Read battery voltage from ADC
    // TODO: Convert ADC value to percentage
    // TODO: Check charging status GPIO

    // Placeholder values
    battery_percentage_ = 85;
    charging_ = false;

    if (battery_callback_) {
        battery_callback_(battery_percentage_, charging_);
    }

    ESP_LOGD(TAG, "Battery: %d%%, Charging: %s", battery_percentage_, charging_ ? "yes" : "no");
}

esp_err_t PowerManager::enterSleep() {
    if (state_ == PowerState::SLEEPING) return ESP_OK;

    ESP_LOGI(TAG, "Entering sleep mode...");
    state_ = PowerState::SLEEPING;

    // TODO: Turn off display
    // TODO: Configure wake sources (any key, etc.)

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // Example: wake on GPIO0 low
    esp_light_sleep_start();

    return ESP_OK;
}

esp_err_t PowerManager::wake() {
    if (state_ != PowerState::SLEEPING) return ESP_OK;

    ESP_LOGI(TAG, "Waking from sleep...");
    state_ = PowerState::ACTIVE;

    // TODO: Turn on display

    return ESP_OK;
}

esp_err_t PowerManager::requestPowerOff() {
    ESP_LOGI(TAG, "Power off requested - show confirmation");
    // TODO: Show confirmation dialog in UI
    return ESP_OK;
}

esp_err_t PowerManager::powerOff() {
    ESP_LOGI(TAG, "Shutting down...");

    // TODO: Ensure SD card is unmounted
    // TODO: Turn off peripherals

    esp_deep_sleep_start();
    return ESP_OK;  // Never reached
}
