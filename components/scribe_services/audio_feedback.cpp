#include "audio_feedback.h"
#include "../scribe_hw/tab5_io_expander.h"
#include <esp_log.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "AUDIO_FEEDBACK";

namespace AudioFeedback {

// State
static bool s_initialized = false;
static bool s_enabled = true;
static bool s_available = false;

// LEDC configuration for tone generation
namespace {
constexpr ledc_timer_t kToneTimer = LEDC_TIMER_1;
constexpr ledc_channel_t kToneChannel = LEDC_CHANNEL_1;
constexpr ledc_mode_t kToneSpeedMode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_bit_t kToneDutyRes = LEDC_TIMER_10_BIT;
constexpr int kToneMaxDuty = (1 << kToneDutyRes) - 1;

// Simple tone sequences (frequency in Hz, duration in ms)
struct Tone {
    int freq;
    int duration_ms;
};

// Sound effect definitions
constexpr Tone kStartupTones[] = {
    {523, 100},  // C5
    {659, 100},  // E5
    {784, 150},  // G5
};

constexpr Tone kShutdownTones[] = {
    {784, 100},  // G5
    {659, 100},  // E5
    {523, 150},  // C5
};

constexpr Tone kClickTones[] = {
    {800, 20},   // Short click
};

constexpr Tone kErrorTones[] = {
    {200, 100},  // Low tone
    {150, 100},  // Lower tone
};

constexpr Tone kSuccessTones[] = {
    {880, 50},   // High tone
    {1100, 50},  // Higher tone
};

constexpr Tone kConnectingTones[] = {
    {440, 50},   // A4
    {0, 50},     // Pause
    {440, 50},   // A4
};

constexpr Tone kConnectedTones[] = {
    {523, 80},   // C5
    {659, 80},   // E5
};

constexpr int kToneVolumePercent = 30;  // 30% volume
}

// Play a single tone
static void playTone(int freq, int duration_ms) {
    if (freq == 0) {
        // Silence
        ledc_set_duty(kToneSpeedMode, kToneChannel, 0);
        ledc_update_duty(kToneSpeedMode, kToneChannel);
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        return;
    }

    // Calculate duty for frequency (50% duty cycle for square wave)
    int duty = (kToneMaxDuty * kToneVolumePercent) / 100;

    // Configure frequency
    ledc_set_freq(kToneSpeedMode, kToneTimer, freq);

    // Set duty and start
    ledc_set_duty(kToneSpeedMode, kToneChannel, duty);
    ledc_update_duty(kToneSpeedMode, kToneChannel);

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    // Stop tone
    ledc_set_duty(kToneSpeedMode, kToneChannel, 0);
    ledc_update_duty(kToneSpeedMode, kToneChannel);
}

// Play a tone sequence
static void playSequence(const Tone* tones, size_t count) {
    for (size_t i = 0; i < count; i++) {
        playTone(tones[i].freq, tones[i].duration_ms);
    }
}

esp_err_t init() {
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing audio feedback...");

    // Check if we have a speaker pin (Tab5 uses GPIO 46 for speaker)
    // For now, we'll use a simple approach without full codec init
    // This can be expanded later to use esp_codec_dev

    // Configure LEDC timer for tone generation
    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode = kToneSpeedMode;
    timer_cfg.duty_resolution = kToneDutyRes;
    timer_cfg.timer_num = kToneTimer;
    timer_cfg.freq_hz = 440;  // Default to A4
    timer_cfg.clk_cfg = LEDC_AUTO_CLK;

    esp_err_t ret = ledc_timer_config(&timer_cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LEDC timer config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure LEDC channel
    ledc_channel_config_t channel_cfg = {};
    channel_cfg.gpio_num = GPIO_NUM_46;  // Tab5 speaker pin
    channel_cfg.speed_mode = kToneSpeedMode;
    channel_cfg.channel = kToneChannel;
    channel_cfg.intr_type = LEDC_INTR_DISABLE;
    channel_cfg.timer_sel = kToneTimer;
    channel_cfg.duty = 0;
    channel_cfg.hpoint = 0;

    ret = ledc_channel_config(&channel_cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LEDC channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_available = true;
    s_initialized = true;
    ESP_LOGI(TAG, "Audio feedback initialized");
    return ESP_OK;
}

esp_err_t play(SoundType type) {
    if (!s_initialized || !s_available || !s_enabled) {
        return ESP_ERR_INVALID_STATE;
    }

    switch (type) {
        case SoundType::STARTUP:
            playSequence(kStartupTones, sizeof(kStartupTones) / sizeof(kStartupTones[0]));
            break;
        case SoundType::SHUTDOWN:
            playSequence(kShutdownTones, sizeof(kShutdownTones) / sizeof(kShutdownTones[0]));
            break;
        case SoundType::CLICK:
            playSequence(kClickTones, sizeof(kClickTones) / sizeof(kClickTones[0]));
            break;
        case SoundType::ERROR:
            playSequence(kErrorTones, sizeof(kErrorTones) / sizeof(kErrorTones[0]));
            break;
        case SoundType::SUCCESS:
            playSequence(kSuccessTones, sizeof(kSuccessTones) / sizeof(kSuccessTones[0]));
            break;
        case SoundType::CONNECTING:
            playSequence(kConnectingTones, sizeof(kConnectingTones) / sizeof(kConnectingTones[0]));
            break;
        case SoundType::CONNECTED:
            playSequence(kConnectedTones, sizeof(kConnectedTones) / sizeof(kConnectedTones[0]));
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

bool isAvailable() {
    return s_available;
}

void setEnabled(bool enabled) {
    s_enabled = enabled;
}

bool isEnabled() {
    return s_enabled;
}

} // namespace AudioFeedback
