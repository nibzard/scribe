#pragma once

#include <esp_err.h>
#include <string>
#include <functional>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Progress callback for typing status
using ExportProgressCallback = std::function<void(size_t chars_sent, size_t total)>;

// "Send to Computer" - USB HID device mode that types the document
// Note: Requires keyboard unplug or dual USB port configuration
class SendToComputer {
public:
    static SendToComputer& getInstance();

    // Initialize USB device mode
    esp_err_t init();

    // Start typing the document
    // Returns ESP_OK immediately, types in background
    esp_err_t start(const std::string& text, ExportProgressCallback callback);

    // Stop typing (user pressed Esc)
    esp_err_t stop();

    // Check if currently typing
    bool isRunning() const { return running_.load(); }

    // Check if device mode is active
    bool isDeviceModeActive() const { return device_mode_active_.load(); }

private:
    SendToComputer() = default;
    ~SendToComputer() = default;

    std::atomic<bool> running_{false};
    std::atomic<bool> device_mode_active_{false};
    std::string text_to_send_;
    size_t send_pos_ = 0;
    ExportProgressCallback progress_callback_;

    TaskHandle_t send_task_handle_{nullptr};
    SemaphoreHandle_t mutex_{nullptr};

    // USB device mode management
    esp_err_t switchToDeviceMode();
    esp_err_t switchToHostMode();

    // Character to HID keycode conversion
    struct HIDKeycode {
        uint8_t keycode;
        uint8_t modifiers;
    };
    HIDKeycode charToHIDKeycode(char c) const;

    // Send HID report
    esp_err_t sendHIDReport(uint8_t keycode, uint8_t modifiers, bool press);
    esp_err_t releaseAllKeys();

    // Send task
    static void sendTask(void* arg);

    // Rate limiting: 60-120 chars/sec to avoid host buffer overflow
    static const int CHARS_PER_SEC = 80;
};
