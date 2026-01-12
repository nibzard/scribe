#include "battery.h"
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_cali.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>

static const char* TAG = "SCRIBE_BATTERY";

// Battery hardware configuration
// Adjust these values based on actual Tab5 hardware design
#define VOLTAGE_DIVIDER_RATIO 2.0f      // Adjust based on hardware divider
#define BATTERY_ADC_CHANNEL    ADC_CHANNEL_0  // GPIO1
#define BATTERY_ADC_UNIT        ADC_UNIT_1
#define CHARGING_GPIO          GPIO_NUM_2     // Charging status pin
#define BATTERY_LOW_PERCENT    20             // Low battery threshold

// ADC configuration
#define ADC_ATTEN       ADC_ATTEN_DB_12
#define ADC_WIDTH       ADC_BITWIDTH_12

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

    // Initialize ADC for battery voltage reading
    adc_oneshot_unit_init_cfg_t adc_init_cfg = {};
    adc_init_cfg.unit_id = BATTERY_ADC_UNIT;
    adc_init_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
#if defined(ADC_RTC_CLK_SRC_DEFAULT)
    adc_init_cfg.clk_src = ADC_RTC_CLK_SRC_DEFAULT;
#elif defined(ADC_DIGI_CLK_SRC_DEFAULT)
    adc_init_cfg.clk_src = ADC_DIGI_CLK_SRC_DEFAULT;
#endif

    esp_err_t ret = adc_oneshot_new_unit(&adc_init_cfg, &adc_handle_);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        // Continue anyway - will use estimated values
        adc_handle_ = nullptr;
    } else {
        // Configure ADC channel
        adc_oneshot_chan_cfg_t chan_cfg = {
            .atten = ADC_ATTEN,
            .bitwidth = ADC_WIDTH,
        };
        ret = adc_oneshot_config_channel(adc_handle_, BATTERY_ADC_CHANNEL, &chan_cfg);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
        }

        // Initialize ADC calibration (optional, improves accuracy)
        adc_cali_curve_fitting_config_t cali_config = {};
        cali_config.unit_id = BATTERY_ADC_UNIT;
        cali_config.atten = ADC_ATTEN;
        cali_config.bitwidth = ADC_WIDTH;
        cali_config.chan = BATTERY_ADC_CHANNEL;
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle_);
        if (ret != ESP_OK) {
            ESP_LOGD(TAG, "ADC calibration not available: %s", esp_err_to_name(ret));
            adc_cali_handle_ = nullptr;
        }
    }

    // Initialize GPIO for charging status detection
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << CHARGING_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
    io_conf.hys_ctrl_mode = GPIO_HYS_SOFT_DISABLE;
#endif
    gpio_config(&io_conf);
    charging_gpio_ = CHARGING_GPIO;

    update();

    ESP_LOGI(TAG, "Battery monitor initialized");
    return ESP_OK;
}

void Battery::deinit() {
    if (adc_handle_) {
        adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
    }

    if (adc_cali_handle_) {
        adc_cali_delete_scheme_curve_fitting(adc_cali_handle_);
        adc_cali_handle_ = nullptr;
    }
}

void Battery::update() {
    voltage_ = readVoltage();
    percentage_ = voltageToPercentage(voltage_);

    // Read charging status from GPIO
    if (charging_gpio_ != GPIO_NUM_NC) {
        // Active low - pin is low when charging
        charging_ = (gpio_get_level(charging_gpio_) == 0);
    } else {
        charging_ = false;
    }

    ESP_LOGD(TAG, "Battery: %d%%, %.2fV, Charging: %s",
             percentage_, voltage_, charging_ ? "yes" : "no");
}

float Battery::readVoltage() {
    if (!adc_handle_) {
        // ADC not available, return estimated value
        return 3.85f;
    }

    int adc_raw = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle_, BATTERY_ADC_CHANNEL, &adc_raw);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read ADC: %s", esp_err_to_name(ret));
        return 3.85f;  // Default estimated value
    }

    // Convert ADC reading to voltage
    int voltage_mv = 0;
    if (adc_cali_handle_) {
        adc_cali_raw_to_voltage(adc_cali_handle_, adc_raw, &voltage_mv);
    } else {
        // Fallback: rough conversion without calibration
        // ADC_ATTEN_DB_12: 0-3.1V range, 12-bit = 4095 steps
        voltage_mv = (adc_raw * 3100) / 4095;
    }

    // Apply voltage divider ratio to get battery voltage
    float battery_voltage = (voltage_mv / 1000.0f) * VOLTAGE_DIVIDER_RATIO;

    ESP_LOGV(TAG, "ADC raw: %d, measured: %.2fV, battery: %.2fV",
             adc_raw, voltage_mv / 1000.0f, battery_voltage);

    return battery_voltage;
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

bool Battery::isLow() const {
    return percentage_ <= BATTERY_LOW_PERCENT;
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
