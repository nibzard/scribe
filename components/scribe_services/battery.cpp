#include "battery.h"
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali_scheme.h>
#include <driver/rtc_io.h>

static const char* TAG = "SCRIBE_BATTERY";

// Battery voltage divider ratio (adjust per actual hardware)
#define VOLTAGE_DIVIDER_RATIO 2.0f

// ADC configuration
#define ADC_ATTEN       ADC_ATTEN_DB_11
#define ADC_CHANNEL     ADC_CHANNEL_0

// Battery percentage calculation
struct BatteryCurve {
    float voltage;
    int percentage;
};

// Approximate LiPo discharge curve
static const BatteryCurve battery_curve[] = {
    {4.2f, 100},
    {4.0f, 90},
    {3.85f, 70},
    {3.7f, 50},
    {3.6f, 30},
    {3.5f, 15},
    {3.3f, 5},
    {3.0f, 0},
};

Battery& Battery::getInstance() {
    static Battery instance;
    return instance;
}

esp_err_t Battery::init() {
    ESP_LOGI(TAG, "Initializing battery monitor...");

    // TODO: Configure ADC for battery voltage reading
    // TODO: Set up GPIO for charging status detection

    update();

    return ESP_OK;
}

void Battery::update() {
    voltage_ = readVoltage();
    percentage_ = voltageToPercentage(voltage_);

    // TODO: Read charging status GPIO
    charging_ = false;

    ESP_LOGD(TAG, "Battery: %d%%, %.2fV, Charging: %s",
             percentage_, voltage_, charging_ ? "yes" : "no");
}

float Battery::readVoltage() {
    // TODO: Configure ADC and read raw value
    // TODO: Convert to voltage using calibration
    return 3.85f;  // Placeholder
}

int Battery::voltageToPercentage(float voltage) {
    // Clamp voltage to valid range
    if (voltage > 4.2f) voltage = 4.2f;
    if (voltage < 3.0f) voltage = 3.0f;

    // Linear interpolation between curve points
    for (size_t i = 0; i < sizeof(battery_curve) / sizeof(BatteryCurve) - 1; i++) {
        if (voltage >= battery_curve[i + 1].voltage) {
            float t = (voltage - battery_curve[i + 1].voltage) /
                      (battery_curve[i].voltage - battery_curve[i + 1].voltage);
            return battery_curve[i + 1].percentage +
                   t * (battery_curve[i].percentage - battery_curve[i + 1].percentage);
        }
    }
    return 0;
}
