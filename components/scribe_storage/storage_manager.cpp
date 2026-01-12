#include "storage_manager.h"
#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include <sdmmc_cmd.h>
#include <sys/stat.h>
#include <dirent.h>

static const char* TAG = "SCRIBE_STORAGE";

StorageManager& StorageManager::getInstance() {
    static StorageManager instance;
    return instance;
}

esp_err_t StorageManager::init() {
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "Initializing SD card...");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = VFS_FAT_MOUNT_DEFAULT_CONFIG();
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;

    sdmmc_card_t* card = nullptr;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = slot_config_.host_id;
    esp_err_t ret = esp_vfs_fat_sdspi_mount(
        "/sdcard",
        &host,
        &slot_config_,
        &mount_config,
        &card);

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
    uint64_t total = 0;
    uint64_t free = 0;
    if (esp_vfs_fat_info("/sdcard", &total, &free) != ESP_OK) {
        return 0;
    }
    return free > SIZE_MAX ? SIZE_MAX : static_cast<size_t>(free);
}
