#include "battery.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_BATTERY";

// Battery percentage calculation
struct BatteryCurve {
    float voltage;
    int percentage;
};

// Approximate LiPo discharge curve (per-cell for 2S pack)
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

    // Initialize IO expander for charge status.
    tab5::IOExpander::getInstance().init();

    // Initialize INA226 power monitor.
    ina226_ready_ = ina226_.init();
    if (ina226_ready_) {
        tab5::Ina226::Config cfg;
        cfg.sampling_rate = tab5::Ina226::Sampling::Rate16;
        cfg.bus_conversion_time = tab5::Ina226::ConversionTime::Us1100;
        cfg.shunt_conversion_time = tab5::Ina226::ConversionTime::Us1100;
        cfg.mode = tab5::Ina226::Mode::ShuntAndBus;
        cfg.shunt_res = 0.005f;
        cfg.max_expected_current = 2.0f;
        ina226_.configure(cfg);
    } else {
        ESP_LOGW(TAG, "INA226 not detected; battery readings will be estimated");
    }

    update();

    ESP_LOGI(TAG, "Battery monitor initialized");
    return ESP_OK;
}

void Battery::deinit() {
    // No dynamic resources to release.
}

void Battery::update() {
    voltage_ = readVoltage();
    percentage_ = voltageToPercentage(voltage_);
    charging_ = tab5::IOExpander::getInstance().isCharging();

    ESP_LOGD(TAG, "Battery: %d%%, %.2fV, Charging: %s",
             percentage_, voltage_, charging_ ? "yes" : "no");
}

float Battery::readVoltage() {
    if (!ina226_ready_) {
        return 3.85f;
    }

    // INA226 reports pack voltage; convert to per-cell voltage for 2S pack.
    float pack_voltage = ina226_.getBusVoltage();
    return pack_voltage * 0.5f;
}

int Battery::voltageToPercentage(float voltage) {
    if (voltage > 4.2f) voltage = 4.2f;
    if (voltage < 3.0f) voltage = 3.0f;

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

bool Battery::isLow() const {
    return percentage_ <= 20;
}

bool Battery::isCritical() const {
    return percentage_ <= 5;
}

int Battery::getPercentage() const {
    return percentage_;
}

float Battery::getVoltage() const {
    return voltage_;
}

bool Battery::isCharging() const {
    return charging_;
}
