// Ensure atomic<bool> is available for thread-safe flags
#pragma once
#include <atomic>

#include <esp_err.h>
#include "sdkconfig.h"

#if defined(CONFIG_ESP_WIFI_REMOTE_ENABLED) && CONFIG_ESP_WIFI_REMOTE_ENABLED
#define SCRIBE_WIFI_REMOTE 1
#else
#define SCRIBE_WIFI_REMOTE 0
#endif

#if SOC_WIFI_SUPPORTED || SCRIBE_WIFI_REMOTE
#define SCRIBE_WIFI_AVAILABLE 1
#else
#define SCRIBE_WIFI_AVAILABLE 0
#endif
#include <functional>
#include <string>
#include <vector>

// WiFi network info
struct WiFiNetwork {
    std::string ssid;
    int rssi;           // Signal strength
    bool secure;        // Whether password required
};

// WiFi status callback
using WiFiStatusCallback = std::function<void(bool connected, const std::string& ssid)>;

// WiFi scan callback
using WiFiScanCallback = std::function<void(const std::vector<WiFiNetwork>& networks)>;

// WiFi manager - handles WiFi connectivity (MVP3 feature)
// Based on SPECS.md section 6.6 - Wi-Fi bring-up
// Opportunistic WiFi: offline is normal, connection is bonus
class WiFiManager {
public:
    static WiFiManager& getInstance();

    // Initialize WiFi
    esp_err_t init();

    // Enable/disable WiFi
    esp_err_t setEnabled(bool enabled);

    // Check if WiFi is enabled
    bool isEnabled() const { return enabled_.load(); }

    // Check if connected
    bool isConnected() const { return connected_.load(); }

    // Get current SSID
    std::string getSSID() const;

    // Start scan for networks (results via callback)
    esp_err_t startScan(WiFiScanCallback callback);

    // Connect to network
    esp_err_t connect(const std::string& ssid, const std::string& password);

    // Disconnect
    esp_err_t disconnect();

    // Register status change callback
    void setStatusCallback(WiFiStatusCallback callback) { status_callback_ = callback; }

    // Get WiFi MAC address
    std::string getMacAddress() const;

    // Get IP address (empty if not connected)
    std::string getIPAddress() const;

    // Internal event handlers (public for C wrapper access)
    void handleWiFiEvent(int32_t event_id, void* event_data);
    void handleIPEvent(int32_t event_id, void* event_data);

private:
    WiFiManager() = default;
    ~WiFiManager() = default;

    std::atomic<bool> enabled_{false};
    std::atomic<bool> connected_{false};
    std::string current_ssid_;
    WiFiScanCallback scan_callback_;
    WiFiStatusCallback status_callback_;

    // Initialize WiFi stack
    esp_err_t initWiFiStack();

    // Configure and start WiFi
    esp_err_t startWiFi();

    // Stop WiFi
    esp_err_t stopWiFi();
};
