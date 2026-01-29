#pragma once

#include <esp_err.h>
#include <atomic>
#include <functional>

class UsbMscDevice {
public:
    static UsbMscDevice& getInstance();

    // Start USB Mass Storage device mode (requires SD card handle).
    esp_err_t start(void* card);

    // Stop USB Mass Storage device mode.
    esp_err_t stop();

    bool isActive() const { return active_.load(); }
    bool isConnected() const { return connected_.load(); }

    void setStatusCallback(std::function<void(bool)> cb) { status_cb_ = cb; }
    void notifyConnection(bool connected);

private:
    UsbMscDevice() = default;
    ~UsbMscDevice() = default;

    void updateConnection(bool connected);

    std::atomic<bool> active_{false};
    std::atomic<bool> connected_{false};
    bool keyboard_was_running_ = false;
    std::function<void(bool)> status_cb_;

    void* storage_handle_ = nullptr;
};
