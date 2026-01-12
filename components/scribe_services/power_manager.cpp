#include "power_manager.h"
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali_scheme.h>
#include <driver/gpio.h>

static const char* TAG = "SCRIBE_POWER";

// Hardware configuration (adjust for Tab5)
#define BATTERY_ADC_CHANNEL     ADC_CHANNEL_0
#define BATTERY_ADC_ATTEN       ADC_ATTEN_DB_11
#define CHARGING_GPIO           GPIO_NUM_NC  // Configure based on hardware
#define WAKE_GPIO_MASK          (1ULL << 0)  // GPIO0 as wake source

// Battery voltage thresholds (in mV)
#define BATTERY_VOLTAGE_MAX     4200
#define BATTERY_VOLTAGE_MIN     3300

PowerManager& PowerManager::getInstance() {
    static PowerManager instance;
    return instance;
}

esp_err_t PowerManager::init() {
    ESP_LOGI(TAG, "Initializing power management...");

    last_activity_time_ = xTaskGetTickCount();

    // Configure ADC for battery monitoring
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC init failed: %s - battery monitoring disabled", esp_err_to_name(ret));
    } else {
        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_12,
            .atten = BATTERY_ADC_ATTEN,
        };
        adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &config);
    }

    // Configure charging status GPIO
    if (CHARGING_GPIO != GPIO_NUM_NC) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << CHARGING_GPIO),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
    }

    updateBatteryStatus();

    return ESP_OK;
}

void PowerManager::updateBatteryStatus() {
    // TODO: Read battery voltage from ADC
    // For now, use placeholder values that can be updated when hardware is available
    int raw_adc = 0;
    float voltage = 0.0f;

    // Calculate battery percentage
    int percentage = 100;
    if (voltage > 0) {
        percentage = (int)((voltage - BATTERY_VOLTAGE_MIN) /
                          (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN) * 100);
        percentage = (percentage < 0) ? 0 : (percentage > 100) ? 100 : percentage;
    }

    battery_percentage_.store(percentage);

    // Check charging status
    bool charging = false;
    if (CHARGING_GPIO != GPIO_NUM_NC) {
        charging = (gpio_get_level(CHARGING_GPIO) == 0);  // Active low typically
    }
    charging_.store(charging);

    if (battery_callback_) {
        battery_callback_(battery_percentage_.load(), charging_.load());
    }

    ESP_LOGD(TAG, "Battery: %d%%, Charging: %s", battery_percentage_.load(),
             charging_.load() ? "yes" : "no");
}

esp_err_t PowerManager::configureWakeSources() {
    // Configure any GPIO as wake source
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // Wake on GPIO0 low

    // Enable touch pad wake (for capacitive buttons)
    // esp_sleep_enable_touchpad_wakeup();

    return ESP_OK;
}

esp_err_t PowerManager::prepareForSleep() {
    ESP_LOGI(TAG, "Preparing for sleep...");

    // TODO: Turn off display backlight
    // TODO: Stop USB host (save power)
    // TODO: Reduce CPU frequency

    // Configure wake sources
    configureWakeSources();

    return ESP_OK;
}

esp_err_t PowerManager::resumeFromSleep() {
    ESP_LOGI(TAG, "Resuming from sleep...");

    // TODO: Turn on display backlight
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

    return ESP_OK;
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
