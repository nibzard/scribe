#pragma once

#include <esp_err.h>
#include <functional>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

// State
enum class PowerState {
    ACTIVE,
    SLEEPING,
    OFF
};

// Auto-sleep timeout options
enum class AutoSleepTimeout {
    OFF,
    MIN_5,
    MIN_15,
    MIN_30
};

// Battery status callback
using BatteryCallback = std::function<void(int percentage, bool charging)>;

// Wake callback (called when device wakes from sleep)
using WakeCallback = std::function<void()>;

// Power management for battery, sleep, shutdown
class PowerManager {
public:
    static PowerManager& getInstance();

    // Initialize power management
    esp_err_t init();

    // Get battery percentage
    int getBatteryPercentage() const { return battery_percentage_.load(); }

    // Check if charging
    bool isCharging() const { return charging_.load(); }

    // Get current power state
    PowerState getState() const { return state_.load(); }

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

    // Register wake callback
    void setWakeCallback(WakeCallback cb) { wake_callback_ = cb; }

    // Check if battery is low
    bool isBatteryLow() const { return battery_percentage_.load() < 15; }

    // Set auto-sleep timeout
    void setAutoSleepTimeout(AutoSleepTimeout timeout) {
        auto_sleep_timeout_ = timeout;
    }

    // Get auto-sleep timeout
    AutoSleepTimeout getAutoSleepTimeout() const { return auto_sleep_timeout_; }

    // Reset activity timer (call on user input)
    void resetActivityTimer();

    // Start activity monitoring
    esp_err_t startActivityMonitoring();

    // Stop activity monitoring
    esp_err_t stopActivityMonitoring();

private:
    PowerManager()
        : battery_percentage_(100), charging_(false), state_(PowerState::ACTIVE),
          auto_sleep_timeout_(AutoSleepTimeout::MIN_15) {}
    ~PowerManager() = default;

    std::atomic<int> battery_percentage_;
    std::atomic<bool> charging_;
    std::atomic<PowerState> state_;
    BatteryCallback battery_callback_;
    WakeCallback wake_callback_;

    // Auto-sleep
    AutoSleepTimeout auto_sleep_timeout_;
    TimerHandle_t activity_timer_{nullptr};
    std::atomic<bool> activity_monitoring_{false};
    TickType_t last_activity_time_;

    // Update battery status from ADC
    void updateBatteryStatus();

    // Activity timer callback
    static void activityTimerCallback(TimerHandle_t timer);

    // Check if should enter sleep based on inactivity
    void checkInactivitySleep();

    // Configure wake sources
    esp_err_t configureWakeSources();

    // Enable/disable peripherals for sleep
    esp_err_t prepareForSleep();
    esp_err_t resumeFromSleep();
};
