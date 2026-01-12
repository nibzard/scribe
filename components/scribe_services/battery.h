#pragma once

#include <esp_err.h>
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali_scheme.h>

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
    Battery() : percentage_(100), voltage_(4.2f), charging_(false),
                adc_handle_(nullptr), adc_cali_handle_(nullptr),
                charging_gpio_(GPIO_NUM_NC) {}
    ~Battery() = default;

    int percentage_;
    float voltage_;
    bool charging_;

    // ADC handle
    adc_oneshot_unit_handle_t adc_handle_;
    adc_cali_handle_t adc_cali_handle_;

    // Charging status GPIO
    gpio_num_t charging_gpio_;

    // Read ADC and calculate voltage
    float readVoltage();

    // Convert voltage to percentage
    int voltageToPercentage(float voltage);
};
