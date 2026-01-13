#pragma once

#include <esp_err.h>
#include "../scribe_hw/tab5_ina226.h"
#include "../scribe_hw/tab5_io_expander.h"

// Battery monitoring using ADC
class Battery {
public:
    static Battery& getInstance();

    // Initialize battery monitoring
    esp_err_t init();

    // Clean up resources
    void deinit();

    // Get battery percentage (0-100)
    int getPercentage() const;

    // Get battery voltage in volts
    float getVoltage() const;

    // Check if battery is low (< 20%)
    bool isLow() const;

    // Check if battery is critical (< 5%)
    bool isCritical() const;

    // Check if device is charging
    bool isCharging() const;

    // Update battery readings
    void update();

private:
    Battery() : percentage_(100), voltage_(4.2f), charging_(false) {}
    ~Battery() = default;

    int percentage_;
    float voltage_;
    bool charging_;

    tab5::Ina226 ina226_{};
    bool ina226_ready_{false};

    float readVoltage();

    // Convert voltage to percentage
    int voltageToPercentage(float voltage);
};
