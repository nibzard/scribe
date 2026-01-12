#pragma once

#include <esp_err.h>
#include <functional>

// State
enum class PowerState {
    ACTIVE,
    SLEEPING,
    OFF
};

// Battery status callback
using BatteryCallback = std::function<void(int percentage, bool charging)>;

// Power management for battery, sleep, shutdown
class PowerManager {
public:
    static PowerManager& getInstance();

    // Initialize power management
    esp_err_t init();

    // Get battery percentage
    int getBatteryPercentage() const { return battery_percentage_; }

    // Check if charging
    bool isCharging() const { return charging_; }

    // Enter sleep mode
    esp_err_t enterSleep();

    // Wake from sleep
    esp_err_t wake();

    // Request power off (shows confirmation first)
    esp_err_t requestPowerOff();

    // Force power off
    esp_err_t powerOff();

    // Register battery status callback
    void setBatteryCallback(BatteryCallback cb) { battery_callback_ = cb; }

    // Check if battery is low
    bool isBatteryLow() const { return battery_percentage_ < 15; }

private:
    PowerManager() : battery_percentage_(100), charging_(false), state_(PowerState::ACTIVE) {}
    ~PowerManager() = default;

    int battery_percentage_;
    bool charging_;
    PowerState state_;
    BatteryCallback battery_callback_;

    // Update battery status from ADC
    void updateBatteryStatus();
};
