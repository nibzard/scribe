#include "screen_diagnostics.h"
#include "../../scribe_services/battery.h"
#include "../../scribe_services/audio_manager.h"
#include "../../scribe_services/imu_manager.h"
#include "../../scribe_services/rtc_time.h"
#include "../../scribe_services/wifi_manager.h"
#include "../../scribe_storage/storage_manager.h"
#include "../../scribe_storage/settings_store.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
#include <esp_log.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_private/esp_clk.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

static const char* TAG = "SCRIBE_SCREEN_DIAGNOSTICS";

static const char* orientationLabel(int value) {
    Strings& strings = Strings::getInstance();
    switch (value) {
        case 0:
            return strings.get("settings.orientation_auto");
        case 1:
            return strings.get("settings.orientation_landscape");
        case 2:
            return strings.get("settings.orientation_portrait");
        case 3:
            return strings.get("settings.orientation_landscape_inverted");
        case 4:
            return strings.get("settings.orientation_portrait_inverted");
        default:
            return strings.get("settings.orientation_auto");
    }
}

ScreenDiagnostics::ScreenDiagnostics() : screen_(nullptr) {
}

ScreenDiagnostics::~ScreenDiagnostics() {
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenDiagnostics::init() {
    ESP_LOGI(TAG, "Initializing diagnostics screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenDiagnostics::createWidgets() {
    // Title
    lv_obj_t* title = lv_label_create(screen_);
    lv_label_set_text(title, Strings::getInstance().get("diagnostics.title"));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    // System info list
    info_list_ = lv_list_create(screen_);
    lv_obj_set_size(info_list_, 380, 350);
    lv_obj_align(info_list_, LV_ALIGN_TOP_MID, 0, 60);
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(info_list_, colors.fg, 0);
    lv_obj_set_style_bg_opa(info_list_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(info_list_, colors.border, 0);
    lv_obj_set_style_border_width(info_list_, 1, 0);

    // Add system info items
    char buf[128];

    // Chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    const char* chip_name = "Unknown";
    switch (chip_info.model) {
        case CHIP_ESP32P4:
            chip_name = "ESP32-P4";
            break;
        case CHIP_ESP32S3:
            chip_name = "ESP32-S3";
            break;
        default:
            break;
    }
    snprintf(buf, sizeof(buf), "Chip: %s %d cores", chip_name, chip_info.cores);
    lv_list_add_button(info_list_, nullptr, buf);

    // CPU frequency
    snprintf(buf, sizeof(buf), "CPU: %d MHz", esp_clk_cpu_freq() / 1000000);
    lv_list_add_button(info_list_, LV_SYMBOL_BARS, buf);

    // Flash size
    uint32_t flash_size = 0;
    if (esp_flash_get_size(nullptr, &flash_size) != ESP_OK) {
        flash_size = 0;
    }
    snprintf(buf, sizeof(buf), "Flash: %lu MB",
             static_cast<unsigned long>(flash_size / (1024U * 1024U)));
    lv_list_add_button(info_list_, LV_SYMBOL_SD_CARD, buf);

    // RAM
    snprintf(buf, sizeof(buf), "Free RAM: %zu KB",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024);
    lv_list_add_button(info_list_, LV_SYMBOL_BATTERY_FULL, buf);

    // Storage
    StorageManager& storage = StorageManager::getInstance();
    snprintf(buf, sizeof(buf), "SD Card: %s",
             storage.isMounted() ? "Mounted" : "Not mounted");
    lv_list_add_button(info_list_, LV_SYMBOL_SD_CARD,
             storage.isMounted() ? "SD Card: Mounted" : "SD Card: Not mounted");

    // Battery
    Battery& battery = Battery::getInstance();
    snprintf(buf, sizeof(buf), "Battery: %d%% %s",
             battery.getPercentage(),
             battery.isCharging() ? "(Charging)" : "");
    lv_list_add_button(info_list_, LV_SYMBOL_CHARGE, buf);

    // Uptime
    snprintf(buf, sizeof(buf), "Uptime: %llu s",
             esp_timer_get_time() / 1000000);
    lv_list_add_button(info_list_, LV_SYMBOL_REFRESH, buf);

    // SDK version
    snprintf(buf, sizeof(buf), "ESP-IDF: %s", esp_get_idf_version());
    lv_list_add_button(info_list_, nullptr, buf);

    // Back button
    lv_list_add_button(info_list_, LV_SYMBOL_LEFT, Strings::getInstance().get("common.back"));

    // Instructions
    lv_obj_t* label = lv_label_create(screen_);
    lv_label_set_text(label, Strings::getInstance().get("diagnostics.instruction"));
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void ScreenDiagnostics::show() {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        const Theme::Colors& colors = Theme::getColors();
        if (info_list_) {
            lv_obj_set_style_bg_color(info_list_, colors.fg, 0);
            lv_obj_set_style_bg_opa(info_list_, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(info_list_, colors.border, 0);
            lv_obj_set_style_border_width(info_list_, 1, 0);
        }
        lv_screen_load(screen_);
        refreshData();
    }
}

void ScreenDiagnostics::hide() {
    if (close_cb_) {
        close_cb_();
    }
}

void ScreenDiagnostics::refreshData() {
    updateSystemInfo();
}

void ScreenDiagnostics::updateSystemInfo() {
    // Clear and rebuild list with current data
    lv_obj_clean(info_list_);

    char buf[128];

    // Free RAM (updated live)
    size_t free_ram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024;
    snprintf(buf, sizeof(buf), "Free RAM: %zu KB", free_ram);
    lv_list_add_button(info_list_, LV_SYMBOL_BATTERY_FULL, buf);

    // Battery (updated live)
    Battery& battery = Battery::getInstance();
    snprintf(buf, sizeof(buf), "Battery: %d%% %s",
             battery.getPercentage(),
             battery.isCharging() ? "(Charging)" : "");
    lv_list_add_button(info_list_, LV_SYMBOL_CHARGE, buf);

    // Uptime (updated live)
    snprintf(buf, sizeof(buf), "Uptime: %llu s",
             esp_timer_get_time() / 1000000);
    lv_list_add_button(info_list_, LV_SYMBOL_REFRESH, buf);

    // Audio + IMU status
    AudioManager& audio = AudioManager::getInstance();
    snprintf(buf, sizeof(buf), "Audio: %s/%s",
             audio.hasSpeaker() ? "Speaker" : "No speaker",
             audio.hasMicrophones() ? "Mics" : "No mics");
    lv_list_add_button(info_list_, LV_SYMBOL_VOLUME_MAX, buf);

    ImuManager& imu = ImuManager::getInstance();
    snprintf(buf, sizeof(buf), "IMU: %s",
             imu.isAvailable() ? "Ready" : "Not detected");
    lv_list_add_button(info_list_, LV_SYMBOL_GPS, buf);

    // Back button
    lv_list_add_button(info_list_, LV_SYMBOL_LEFT, Strings::getInstance().get("common.back"));
}

bool ScreenDiagnostics::exportLogs() {
    ESP_LOGI(TAG, "Exporting system logs...");

    StorageManager& storage = StorageManager::getInstance();
    if (!storage.isMounted()) {
        ESP_LOGW(TAG, "Storage not mounted");
        return false;
    }

    struct stat st;
    if (stat(SCRIBE_LOGS_DIR, &st) != 0) {
        mkdir(SCRIBE_LOGS_DIR, 0755);
    }

    std::string timestamp = getCurrentTimeISO();
    std::string filename = timestamp;
    for (char& c : filename) {
        if (c == ':') {
            c = '-';
        }
    }

    std::string path = std::string(SCRIBE_LOGS_DIR) + "/diagnostics-" + filename + ".txt";
    FILE* f = fopen(path.c_str(), "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open diagnostics file");
        return false;
    }

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    const char* chip_name = "Unknown";
    switch (chip_info.model) {
        case CHIP_ESP32P4:
            chip_name = "ESP32-P4";
            break;
        case CHIP_ESP32S3:
            chip_name = "ESP32-S3";
            break;
        default:
            break;
    }

    uint32_t flash_size = 0;
    if (esp_flash_get_size(nullptr, &flash_size) != ESP_OK) {
        flash_size = 0;
    }

    Battery& battery = Battery::getInstance();
    WiFiManager& wifi = WiFiManager::getInstance();
    AudioManager& audio = AudioManager::getInstance();
    ImuManager& imu = ImuManager::getInstance();

    AppSettings settings;
    SettingsStore::getInstance().load(settings);

    fprintf(f, "Scribe diagnostics\n");
    fprintf(f, "Timestamp: %s\n\n", timestamp.c_str());

    fprintf(f, "System\n");
    fprintf(f, "Chip: %s (%d cores)\n", chip_name, chip_info.cores);
    fprintf(f, "CPU: %d MHz\n", esp_clk_cpu_freq() / 1000000);
    fprintf(f, "Flash: %lu MB\n", static_cast<unsigned long>(flash_size / (1024U * 1024U)));
    fprintf(f, "Free RAM: %zu KB\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024);
    fprintf(f, "Uptime: %llu s\n", esp_timer_get_time() / 1000000);
    fprintf(f, "ESP-IDF: %s\n\n", esp_get_idf_version());

    fprintf(f, "Storage\n");
    fprintf(f, "SD Card: %s\n", storage.isMounted() ? "Mounted" : "Not mounted");
    fprintf(f, "Free space: %zu MB\n", storage.getFreeSpace() / (1024U * 1024U));
    fprintf(f, "\n");

    fprintf(f, "Battery\n");
    fprintf(f, "Charge: %d%% %s\n\n", battery.getPercentage(),
            battery.isCharging() ? "(Charging)" : "");

    fprintf(f, "WiFi\n");
    fprintf(f, "Enabled: %s\n", wifi.isEnabled() ? "yes" : "no");
    fprintf(f, "Connected: %s\n", wifi.isConnected() ? "yes" : "no");
    if (wifi.isConnected()) {
        std::string ssid = wifi.getSSID();
        if (!ssid.empty()) {
            fprintf(f, "SSID: %s\n", ssid.c_str());
        }
    }
    fprintf(f, "\n");

    fprintf(f, "Audio/IMU\n");
    fprintf(f, "Speaker: %s\n", audio.hasSpeaker() ? "yes" : "no");
    fprintf(f, "Microphones: %s\n", audio.hasMicrophones() ? "yes" : "no");
    fprintf(f, "IMU: %s\n", imu.isAvailable() ? "ready" : "not detected");
    fprintf(f, "\n");

    fprintf(f, "Settings\n");
    fprintf(f, "Theme: %s\n", settings.theme_id.c_str());
    fprintf(f, "Font size: %d\n", settings.font_size);
    fprintf(f, "Keyboard layout: %d\n", settings.keyboard_layout);
    fprintf(f, "Auto-sleep: %d\n", settings.auto_sleep);
    fprintf(f, "Orientation: %s\n", orientationLabel(settings.display_orientation));
    fprintf(f, "Backup enabled: %s\n", settings.backup_enabled ? "yes" : "no");
    fprintf(f, "WiFi enabled: %s\n", settings.wifi_enabled ? "yes" : "no");

    fflush(f);
    fsync(fileno(f));
    fclose(f);

    ESP_LOGI(TAG, "Logs exported to %s", path.c_str());
    return true;
}
