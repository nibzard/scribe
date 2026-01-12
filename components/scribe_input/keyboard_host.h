#pragma once

#include "key_event.h"
#include <esp_err.h>
#include <functional>

// Callback type for key events
using KeyCallback = std::function<void(const KeyEvent&)>;

// USB HID host keyboard driver
class KeyboardHost {
public:
    static KeyboardHost& getInstance();

    // Initialize USB HID host
    esp_err_t init();

    // Register callback for key events
    void setCallback(KeyCallback callback) { callback_ = callback; }

    // Start processing keyboard input
    esp_err_t start();

    // Stop processing
    esp_err_t stop();

    bool isConnected() const { return connected_; }

private:
    KeyboardHost() = default;
    ~KeyboardHost() = default;

    KeyCallback callback_;
    bool connected_ = false;
    bool running_ = false;

    // HID report parsing
    void parseHIDReport(const uint8_t* report, size_t len);

    // USB HID host callback
    static void usbEventCallback(const usb_host_event_msg_t* event_msg, void* arg);
};
