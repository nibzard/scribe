#pragma once

#include <esp_err.h>
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>

struct ImuSample {
    float accel_x = 0.0f;  // g
    float accel_y = 0.0f;
    float accel_z = 0.0f;
    float gyro_x = 0.0f;   // deg/s
    float gyro_y = 0.0f;
    float gyro_z = 0.0f;
};

class ImuManager {
public:
    using ShakeCallback = std::function<void()>;

    static ImuManager& getInstance();

    esp_err_t init();
    bool isAvailable() const { return initialized_.load(); }
    bool hasSample() const { return has_sample_.load(); }

    ImuSample getSample() const;

    void setShakeCallback(ShakeCallback cb);
    void setShakeEnabled(bool enabled);
    bool isShakeEnabled() const { return shake_enabled_.load(); }

private:
    ImuManager() = default;
    ~ImuManager() = default;

    static void imuTask(void* arg);
    void updateSample();
    void evaluateShake(const ImuSample& sample);

    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> shake_enabled_{false};
    std::atomic<bool> has_sample_{false};
    ShakeCallback shake_cb_;

    mutable std::mutex sample_mutex_;
    ImuSample last_sample_;

    int64_t last_shake_us_ = 0;
    float shake_threshold_g_ = 1.6f;
    int64_t shake_debounce_us_ = 600 * 1000;
};
