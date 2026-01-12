#include "storage_manager.h"
#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>

static const char* TAG = "SCRIBE_STORAGE";

// SPI pin configuration for Tab5 (adjust per actual hardware)
#define PIN_MISO 37
#define PIN_MOSI 35
#define PIN_CLK  36
#define PIN_CS   34

StorageManager& StorageManager::getInstance() {
    static StorageManager instance;
    return instance;
}

esp_err_t StorageManager::init() {
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "Initializing SD card...");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card = nullptr;
    esp_err_t ret = esp_vfs_fat_sdspi_mount(
        "/sdcard",
        &mount_config,
        &slot_config_,
        &card
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return ret;
    }

    card_ = card;
    sdmmc_card_print_info(stdout, card_);
    mounted_ = true;

    ret = createDirectories();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create directories: %s", esp_err_to_name(ret));
        unmount();
        return ret;
    }

    ESP_LOGI(TAG, "SD card mounted at /sdcard");
    return ESP_OK;
}

esp_err_t StorageManager::createDirectories() {
    const char* dirs[] = {
        SCRIBE_BASE_PATH,
        SCRIBE_PROJECTS_DIR,
        SCRIBE_ARCHIVE_DIR,
        SCRIBE_LOGS_DIR,
        SCRIBE_EXPORTS_DIR,
        nullptr
    };

    for (int i = 0; dirs[i] != nullptr; i++) {
        struct stat st;
        if (stat(dirs[i], &st) != 0) {
            if (mkdir(dirs[i], 0755) != 0) {
                ESP_LOGE(TAG, "Failed to create directory: %s", dirs[i]);
                return ESP_FAIL;
            }
            ESP_LOGI(TAG, "Created directory: %s", dirs[i]);
        }
    }

    return ESP_OK;
}

esp_err_t StorageManager::unmount() {
    if (!mounted_) return ESP_ERR_INVALID_STATE;

    esp_err_t ret = esp_vfs_fat_sdcard_unmount("/sdcard", card_);
    if (ret == ESP_OK) {
        mounted_ = false;
        ESP_LOGI(TAG, "SD card unmounted");
    }
    return ret;
}

size_t StorageManager::getFreeSpace() const {
    struct statvfs stat;
    if (statvfs("/sdcard", &stat) != 0) {
        return 0;
    }
    return static_cast<size_t>(stat.f_bsize) * static_cast<size_t>(stat.f_bavail);
}
