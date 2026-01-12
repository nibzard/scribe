#pragma once

#include "key_event.h"
#include <esp_err.h>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <usb/hid_host.h>
#include <atomic>

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

    bool isConnected() const { return connected_.load(); }

private:
    KeyboardHost() = default;
    ~KeyboardHost() = default;

    KeyCallback callback_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};

    // HID device handle
    hid_host_device_handle_t hid_device_handle_{nullptr};
    SemaphoreHandle_t task_semaphore_{nullptr};

    // HID report parsing
    void parseHIDReport(const uint8_t* report, size_t len);

    // USB HID host callback
    static void usbEventCallback(const usb_host_event_msg_t* event_msg, void* arg);

    // HID keyboard callback
    static void hidHostCallback(hid_host_device_handle_t hid_device_handle,
                                const hid_host_event_t event, void* arg);

    // Open HID keyboard device
    esp_err_t openKeyboardDevice(uint8_t address);

    // Close HID keyboard device
    esp_err_t closeKeyboardDevice();

    // Task entry point
    static void keyboardHostTask(void* arg);

    // Process keyboard events
    void processEvents();
};
