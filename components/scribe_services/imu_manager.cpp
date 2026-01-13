#include "imu_manager.h"
#include "../scribe_hw/tab5_i2c.h"
#include <accel_gyro_bmi270.h>
#include <bmi2.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cmath>

static const char* TAG = "SCRIBE_IMU";

namespace {
constexpr float kAccelScaleG = 1.0f / 8192.0f;  // +/-4g
constexpr float kGyroScaleDps = 1.0f / 32.768f; // +/-1000 dps
constexpr int kPollIntervalMs = 50;
}  // namespace

ImuManager& ImuManager::getInstance() {
    static ImuManager instance;
    return instance;
}

esp_err_t ImuManager::init() {
    if (initialized_.load()) {
        return ESP_OK;
    }

    tab5::I2CBus& bus = tab5::I2CBus::getInstance();
    esp_err_t ret = bus.init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = accel_gyro_bmi270_init(bus.handle());
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BMI270 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    accel_gyro_bmi270_enable_sensor();
    updateSample();
    initialized_.store(true);

    running_.store(true);
    if (xTaskCreate(imuTask, "imu_task", 4096, this, 5, nullptr) != pdPASS) {
        running_.store(false);
        ESP_LOGE(TAG, "Failed to start IMU task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "IMU initialized");
    return ESP_OK;
}

ImuSample ImuManager::getSample() const {
    std::lock_guard<std::mutex> lock(sample_mutex_);
    return last_sample_;
}

void ImuManager::setShakeCallback(ShakeCallback cb) {
    shake_cb_ = std::move(cb);
}

void ImuManager::setShakeEnabled(bool enabled) {
    shake_enabled_.store(enabled);
}

void ImuManager::imuTask(void* arg) {
    ImuManager* manager = static_cast<ImuManager*>(arg);
    while (manager->running_.load()) {
        manager->updateSample();
        vTaskDelay(pdMS_TO_TICKS(kPollIntervalMs));
    }
    vTaskDelete(nullptr);
}

void ImuManager::updateSample() {
    struct bmi2_sens_data data = {};
    accel_gyro_bmi270_get_data(&data);

    ImuSample sample;
    sample.accel_x = data.acc.y * kAccelScaleG;
    sample.accel_y = -data.acc.x * kAccelScaleG;
    sample.accel_z = -data.acc.z * kAccelScaleG;
    sample.gyro_x = data.gyr.y * kGyroScaleDps;
    sample.gyro_y = data.gyr.x * kGyroScaleDps;
    sample.gyro_z = -data.gyr.z * kGyroScaleDps;

    {
        std::lock_guard<std::mutex> lock(sample_mutex_);
        last_sample_ = sample;
    }
    has_sample_.store(true);

    if (shake_enabled_.load()) {
        evaluateShake(sample);
    }
}

void ImuManager::evaluateShake(const ImuSample& sample) {
    float mag = std::sqrt(sample.accel_x * sample.accel_x +
                          sample.accel_y * sample.accel_y +
                          sample.accel_z * sample.accel_z);
    float delta = std::fabs(mag - 1.0f);
    if (delta < shake_threshold_g_) {
        return;
    }

    int64_t now_us = esp_timer_get_time();
    if (now_us - last_shake_us_ < shake_debounce_us_) {
        return;
    }

    last_shake_us_ = now_us;
    if (shake_cb_) {
        shake_cb_();
    }
}
