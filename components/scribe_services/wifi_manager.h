#pragma once

#include <esp_err.h>
#include <stdbool.h>

// WiFi manager - handles WiFi connectivity (MVP2 feature)
// Based on SPECS.md section 6.6 - Wi-Fi bring-up

class WiFiManager {
public:
    static WiFiManager& getInstance();

    // Initialize WiFi
    esp_err_t init();

    // Enable/disable WiFi
    esp_err_t setEnabled(bool enabled);

    // Check if WiFi is enabled
    bool isEnabled() const { return enabled_; }

    // Check if connected
    bool isConnected() const { return connected_; }

    // Get current SSID
    const char* getSSID() const { return ssid_; }

    // Start scan for networks
    esp_err_t startScan();

    // Connect to network
    esp_err_t connect(const char* ssid, const char* password);

    // Disconnect
    esp_err_t disconnect();

private:
    WiFiManager() : enabled_(false), connected_(false), ssid_(nullptr) {}
    ~WiFiManager() = default;

    bool enabled_;
    bool connected_;
    const char* ssid_;
};
