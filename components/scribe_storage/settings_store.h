#pragma once

#include <esp_err.h>
#include <string>

// Application settings persistence
// Stores user preferences in /Scribe/settings.json per SPECS.md section 5

struct AppSettings {
    // Theme settings
    bool dark_theme = false;

    // Font size: 0=small, 1=medium, 2=large
    int font_size = 1;

    // Keyboard layout (future expansion)
    int keyboard_layout = 0;  // 0=US

    // Auto-sleep: 0=off, 1=5min, 2=15min, 3=30min
    int auto_sleep = 2;  // Default 15 minutes

    // Display orientation: 0=auto (IMU), 1=landscape, 2=portrait
    int display_orientation = 1;

    // WiFi enabled
    bool wifi_enabled = false;

    // Cloud backup enabled
    bool backup_enabled = false;
};

class SettingsStore {
public:
    static SettingsStore& getInstance();

    // Initialize settings store
    esp_err_t init();

    // Load settings from NVS
    esp_err_t load(AppSettings& settings);

    // Save settings to NVS
    esp_err_t save(const AppSettings& settings);

private:
    SettingsStore() = default;
    ~SettingsStore() = default;
};
