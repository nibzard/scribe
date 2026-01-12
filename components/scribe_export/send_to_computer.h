#pragma once

#include <esp_err.h>
#include <string>
#include <functional>

// Progress callback for typing status
using ExportProgressCallback = std::function<void(size_t chars_sent, size_t total)>;

// "Send to Computer" - USB HID device mode that types the document
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
    bool isRunning() const { return running_; }

private:
    SendToComputer() = default;
    ~SendToComputer() = default;

    bool running_ = false;
    std::string text_to_send_;
    size_t send_pos_ = 0;
    ExportProgressCallback progress_callback_;

    // USB HID device callback
    static void sendNextChar();

    // Rate limiting: 60-120 chars/sec to avoid host buffer overflow
    static const int CHARS_PER_SEC = 80;
};
