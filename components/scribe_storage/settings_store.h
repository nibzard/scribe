#pragma once

#include <esp_err.h>
#include <string>

// Application settings persistence
// Stores user preferences in /Scribe/settings.json per SPECS.md section 5

struct AppSettings {
    // Theme settings
    std::string theme_id = "dracula";

    // Editor font family id
    std::string editor_font_id = "montserrat";

    // Editor font size in pixels
    int font_size = 16;

    // Editor margins: 0=small, 1=medium, 2=large
    int editor_margin = 1;

    // UI scale in percent (50 = current baseline)
    int ui_scale = 50;

    // Keyboard layout (future expansion)
    int keyboard_layout = 0;  // 0=US

    // Auto-sleep: 0=off, 1=5min, 2=15min, 3=30min
    int auto_sleep = 2;  // Default 15 minutes

    // Display orientation: 0=auto (IMU), 1=landscape, 2=portrait,
    // 3=landscape inverted, 4=portrait inverted
    int display_orientation = 0;

    // WiFi enabled
    bool wifi_enabled = false;

    // Screen brightness (0-100)
    int brightness = 50;

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
