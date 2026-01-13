#pragma once

#include <esp_err.h>
#include <driver/sdmmc_host.h>
#if CONFIG_SCRIBE_WOKWI_SIM
#include <driver/sdspi_host.h>
#endif
#include "sdkconfig.h"

// Storage paths
#define SCRIBE_BASE_PATH "/sdcard/Scribe"
#define SCRIBE_PROJECTS_DIR SCRIBE_BASE_PATH "/Projects"
#define SCRIBE_ARCHIVE_DIR SCRIBE_BASE_PATH "/Archive"
#define SCRIBE_LOGS_DIR SCRIBE_BASE_PATH "/Logs"
#define SCRIBE_EXPORTS_DIR SCRIBE_BASE_PATH "/Exports"

// File paths
#define SCRIBE_LIBRARY_JSON SCRIBE_BASE_PATH "/library.json"
#define SCRIBE_SETTINGS_JSON SCRIBE_BASE_PATH "/settings.json"

// SDMMC pin configuration for Tab5 SD card (4-bit)
#define PIN_SD_D0  39
#define PIN_SD_D1  40
#define PIN_SD_D2  41
#define PIN_SD_D3  42
#define PIN_SD_CMD 44
#define PIN_SD_CLK 43
#define SDMMC_BUS_WIDTH 4
#define SDMMC_LDO_CHAN 4

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
        : mounted_(false),
          card_(nullptr)
#if CONFIG_SCRIBE_WOKWI_SIM
          , bus_initialized_(false),
          slot_config_(SDSPI_DEVICE_CONFIG_DEFAULT())
#endif
    {
#if CONFIG_SCRIBE_WOKWI_SIM
        slot_config_.host_id = SPI3_HOST;
        slot_config_.gpio_cs = static_cast<gpio_num_t>(PIN_SD_D3);
        slot_config_.gpio_cd = GPIO_NUM_NC;
        slot_config_.gpio_wp = GPIO_NUM_NC;
        slot_config_.gpio_int = GPIO_NUM_NC;
#endif
    }
    ~StorageManager() = default;

    bool mounted_;
    sdmmc_card_t* card_;
#if CONFIG_SCRIBE_WOKWI_SIM
    bool bus_initialized_;
    sdspi_device_config_t slot_config_;
#endif

    // Create Scribe directory structure
    esp_err_t createDirectories();
};
