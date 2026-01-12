#pragma once

#include <esp_err.h>
#include <driver/sdmmc_host.h>
#include <driver/sdspi_host.h>

// Storage paths
#define SCRIBE_BASE_PATH "/sdcard/Scribe"
#define SCRIBE_PROJECTS_DIR SCRIBE_BASE_PATH "/Projects"
#define SCRIBE_ARCHIVE_DIR SCRIBE_BASE_PATH "/Archive"
#define SCRIBE_LOGS_DIR SCRIBE_BASE_PATH "/Logs"
#define SCRIBE_EXPORTS_DIR SCRIBE_BASE_PATH "/Exports"

// File paths
#define SCRIBE_LIBRARY_JSON SCRIBE_BASE_PATH "/library.json"
#define SCRIBE_SETTINGS_JSON SCRIBE_BASE_PATH "/settings.json"

// SPI pin configuration for Tab5 (adjust per actual hardware)
#define PIN_MISO 37
#define PIN_MOSI 35
#define PIN_CLK  36
#define PIN_CS   34

// Storage manager handles SD card mount and basic file operations
class StorageManager {
public:
    static StorageManager& getInstance();

    // Initialize SD card and create directory structure
    esp_err_t init();

    // Check if SD card is mounted and accessible
    bool isMounted() const { return mounted_; }

    // Unmount SD card
    esp_err_t unmount();

    // Get free space in bytes
    size_t getFreeSpace() const;

private:
    StorageManager()
        : mounted_(false), card_(nullptr), slot_config_(SDSPI_DEVICE_CONFIG_DEFAULT()) {
        slot_config_.host_id = SPI3_HOST;
        slot_config_.gpio_cs = static_cast<gpio_num_t>(PIN_CS);
        slot_config_.gpio_cd = GPIO_NUM_NC;
        slot_config_.gpio_wp = GPIO_NUM_NC;
        slot_config_.gpio_int = GPIO_NUM_NC;
    }
    ~StorageManager() = default;

    bool mounted_;
    sdmmc_card_t* card_;
    sdspi_device_config_t slot_config_;

    // Create Scribe directory structure
    esp_err_t createDirectories();
};
