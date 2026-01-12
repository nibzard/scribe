#pragma once

#include <esp_err.h>

// Battery monitoring using ADC
class Battery {
public:
    static Battery& getInstance();

    // Initialize battery monitoring
    esp_err_t init();

    // Get battery percentage (0-100)
    int getPercentage() const { return percentage_; }

    // Get battery voltage in volts
    float getVoltage() const { return voltage_; }

    // Check if battery is low (< 20%)
    bool isLow() const { return percentage_ < 20; }

    // Check if device is charging
    bool isCharging() const { return charging_; }

    // Update battery readings
    void update();

private:
    Battery() : percentage_(100), voltage_(4.2f), charging_(false) {}
    ~Battery() = default;

    int percentage_;
    float voltage_;
    bool charging_;

    // Read ADC and calculate voltage
    float readVoltage();

    // Convert voltage to percentage
    int voltageToPercentage(float voltage);
};
