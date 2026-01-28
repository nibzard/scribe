#include "storage_manager.h"
#include <esp_log.h>
#include <esp_vfs_fat.h>
#if CONFIG_SCRIBE_WOKWI_SIM
#include <driver/sdspi_host.h>
#include <driver/spi_master.h>
#include <driver/spi_common.h>
#else
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif
#include <sdmmc_cmd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

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
#if CONFIG_SCRIBE_WOKWI_SIM
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = slot_config_.host_id;

    if (!bus_initialized_) {
        spi_bus_config_t bus_cfg = {};
        bus_cfg.mosi_io_num = static_cast<gpio_num_t>(PIN_SD_CMD);
        bus_cfg.miso_io_num = static_cast<gpio_num_t>(PIN_SD_D0);
        bus_cfg.sclk_io_num = static_cast<gpio_num_t>(PIN_SD_CLK);
        bus_cfg.quadwp_io_num = GPIO_NUM_NC;
        bus_cfg.quadhd_io_num = GPIO_NUM_NC;
        bus_cfg.max_transfer_sz = 16 * 1024;

        esp_err_t ret = spi_bus_initialize(slot_config_.host_id, &bus_cfg, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
            return ret;
        }
        bus_initialized_ = true;
    }

    esp_err_t ret = esp_vfs_fat_sdspi_mount(
        "/sdcard",
        &host,
        &slot_config_,
        &mount_config,
        &card);
#else
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = SDMMC_LDO_CHAN,
    };
    static sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;
    if (!pwr_ctrl_handle) {
        esp_err_t ldo_ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
        if (ldo_ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init SDMMC LDO: %s", esp_err_to_name(ldo_ret));
            return ldo_ret;
        }
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = SDMMC_BUS_WIDTH;
    slot_config.clk = static_cast<gpio_num_t>(PIN_SD_CLK);
    slot_config.cmd = static_cast<gpio_num_t>(PIN_SD_CMD);
    slot_config.d0 = static_cast<gpio_num_t>(PIN_SD_D0);
    slot_config.d1 = static_cast<gpio_num_t>(PIN_SD_D1);
    slot_config.d2 = static_cast<gpio_num_t>(PIN_SD_D2);
    slot_config.d3 = static_cast<gpio_num_t>(PIN_SD_D3);

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(
        "/sdcard",
        &host,
        &slot_config,
        &mount_config,
        &card);
#endif

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
#if CONFIG_SCRIBE_WOKWI_SIM
        if (bus_initialized_) {
            spi_bus_free(slot_config_.host_id);
            bus_initialized_ = false;
        }
#endif
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
        if (stat(dirs[i], &st) == 0) {
            if (!S_ISDIR(st.st_mode)) {
                ESP_LOGE(TAG, "Path exists but is not a directory: %s", dirs[i]);
                return ESP_FAIL;
            }
            continue;
        }

        if (errno != ENOENT) {
            ESP_LOGE(TAG, "Failed to stat directory %s: %s", dirs[i], strerror(errno));
            return ESP_FAIL;
        }

        if (mkdir(dirs[i], 0755) != 0 && errno != EEXIST) {
            ESP_LOGE(TAG, "Failed to create directory %s: %s", dirs[i], strerror(errno));
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Created directory: %s", dirs[i]);
    }

    return ESP_OK;
}

esp_err_t StorageManager::ensureDirectories() {
    if (!mounted_) {
        ESP_LOGE(TAG, "SD card not mounted; cannot ensure directories");
        return ESP_ERR_INVALID_STATE;
    }
    return createDirectories();
}

esp_err_t StorageManager::unmount() {
    if (!mounted_) return ESP_ERR_INVALID_STATE;

    esp_err_t ret = esp_vfs_fat_sdcard_unmount("/sdcard", card_);
    if (ret == ESP_OK) {
        mounted_ = false;
        ESP_LOGI(TAG, "SD card unmounted");
    }

#if CONFIG_SCRIBE_WOKWI_SIM
    if (bus_initialized_) {
        spi_bus_free(slot_config_.host_id);
        bus_initialized_ = false;
    }
#endif
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
