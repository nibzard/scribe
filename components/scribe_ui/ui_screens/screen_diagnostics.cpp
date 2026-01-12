#include "screen_diagnostics.h"
#include "../../scribe_services/battery.h"
#include "../../scribe_storage/storage_manager.h"
#include <esp_log.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "SCRIBE_SCREEN_DIAGNOSTICS";

ScreenDiagnostics::ScreenDiagnostics() : screen_(nullptr) {
}

ScreenDiagnostics::~ScreenDiagnostics() {
    if (screen_) {
        lv_obj_del(screen_);
    }
}

void ScreenDiagnostics::init() {
    ESP_LOGI(TAG, "Initializing diagnostics screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(screen_, lv_color_white(), 0);

    createWidgets();
}

void ScreenDiagnostics::createWidgets() {
    // Title
    lv_obj_t* title = lv_label_create(screen_);
    lv_label_set_text(title, "Diagnostics");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    // System info list
    info_list_ = lv_list_create(screen_);
    lv_obj_set_size(info_list_, 380, 350);
    lv_obj_align(info_list_, LV_ALIGN_TOP_MID, 0, 60);

    // Add system info items
    char buf[128];

    // Chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    snprintf(buf, sizeof(buf), "Chip: %s %d cores",
             chip_info.model == CHIP_ESP32S3 ? "ESP32-S3" : "Unknown",
             chip_info.cores);
    lv_list_add_btn(info_list_, nullptr, buf);

    // CPU frequency
    snprintf(buf, sizeof(buf), "CPU: %d MHz", apb_cpu_get_hz_mhz() * 80);
    lv_list_add_btn(info_list_, LV_SYMBOL_CPU, buf);

    // Flash size
    snprintf(buf, sizeof(buf), "Flash: %zu MB",
             spi_flash_get_chip_size() / (1024 * 1024));
    lv_list_add_btn(info_list_, LV_SYMBOL_SD, buf);

    // RAM
    snprintf(buf, sizeof(buf), "Free RAM: %zu KB",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024);
    lv_list_add_btn(info_list_, LV_SYMBOL_BATTERY_FULL, buf);

    // Storage
    StorageManager& storage = StorageManager::getInstance();
    snprintf(buf, sizeof(buf), "SD Card: %s",
             storage.isMounted() ? "Mounted" : "Not mounted");
    lv_list_add_btn(info_list_, LV_SYMBOL_SD,
             storage.isMounted() ? "SD Card: Mounted" : "SD Card: Not mounted");

    // Battery
    Battery& battery = Battery::getInstance();
    snprintf(buf, sizeof(buf), "Battery: %d%% %s",
             battery.getPercentage(),
             battery.isCharging() ? "(Charging)" : "");
    lv_list_add_btn(info_list_, LV_SYMBOL_CHARGE, buf);

    // Uptime
    snprintf(buf, sizeof(buf), "Uptime: %llu s",
             esp_timer_get_time() / 1000000);
    lv_list_add_btn(info_list_, LV_SYMBOL_TIME, buf);

    // SDK version
    snprintf(buf, sizeof(buf), "ESP-IDF: %s", esp_get_idf_version());
    lv_list_add_btn(info_list_, nullptr, buf);

    // Back button
    lv_obj_t* btn = lv_list_add_btn(info_list_, LV_SYMBOL_LEFT, "Back");

    // Instructions
    lv_obj_t* label = lv_label_create(screen_);
    lv_label_set_text(label, "Press Enter to export logs | Esc to close");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void ScreenDiagnostics::show() {
    if (screen_) {
        lv_scr_load(screen_);
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
    lv_list_add_btn(info_list_, LV_SYMBOL_BATTERY_FULL, buf);

    // Battery (updated live)
    Battery& battery = Battery::getInstance();
    snprintf(buf, sizeof(buf), "Battery: %d%% %s",
             battery.getPercentage(),
             battery.isCharging() ? "(Charging)" : "");
    lv_list_add_btn(info_list_, LV_SYMBOL_CHARGE, buf);

    // Uptime (updated live)
    snprintf(buf, sizeof(buf), "Uptime: %llu s",
             esp_timer_get_time() / 1000000);
    lv_list_add_btn(info_list_, LV_SYMBOL_TIME, buf);

    // Back button
    lv_list_add_btn(info_list_, LV_SYMBOL_LEFT, "Back");
}

void ScreenDiagnostics::exportLogs() {
    ESP_LOGI(TAG, "Exporting system logs...");

    // TODO: Collect logs and export to SD card
    // - Read from UART debug log buffer
    // - Write to /sdcard/Scribe/Logs/diagnostics.txt

    ESP_LOGI(TAG, "Logs exported");
}
