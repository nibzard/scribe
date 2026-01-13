#pragma once

#include <esp_err.h>
#include <esp_codec_dev.h>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

// Audio codec manager for Tab5 (ES8388 speaker + ES7210 microphones).
class AudioManager {
public:
    static AudioManager& getInstance();

    // Initialize I2S + codec devices.
    esp_err_t init();

    bool isAvailable() const { return initialized_.load(); }
    bool hasSpeaker() const { return speaker_ready_.load(); }
    bool hasMicrophones() const { return mic_ready_.load(); }

    esp_err_t setSpeakerVolume(uint8_t volume);
    uint8_t getSpeakerVolume() const { return speaker_volume_; }

    // Record raw interleaved PCM samples (4-channel, 16-bit, 48 kHz).
    esp_err_t record(std::vector<int16_t>& data, uint32_t duration_ms, float gain = 80.0f);

    // Play interleaved PCM samples (stereo, 16-bit, 48 kHz).
    esp_err_t play(const std::vector<int16_t>& data, bool async = true);

private:
    AudioManager() = default;
    ~AudioManager() = default;

    esp_err_t playBlocking(const std::vector<int16_t>& data);
    esp_err_t playBlockingMutable(std::vector<int16_t>& data);
    static void playTask(void* arg);

    std::atomic<bool> initialized_{false};
    std::atomic<bool> speaker_ready_{false};
    std::atomic<bool> mic_ready_{false};
    std::atomic<bool> playing_{false};
    uint8_t speaker_volume_ = 60;

    std::mutex play_mutex_;
    std::vector<int16_t> play_buffer_;
};
