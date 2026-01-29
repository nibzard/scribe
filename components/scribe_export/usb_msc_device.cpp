#include "usb_msc_device.h"
#include "usb_hid_device.h"
#include "sdkconfig.h"
#include "tinyusb.h"
#include "tinyusb_msc.h"
#include "tinyusb_default_config.h"
#include "../scribe_input/keyboard_host_control.h"
#include <sdmmc_cmd.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <tusb.h>

static const char* TAG = "SCRIBE_USB_MSC";

static const tusb_desc_device_t kMscDeviceDescriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
#if CONFIG_TINYUSB_DESC_USE_ESPRESSIF_VID
    .idVendor = TINYUSB_ESPRESSIF_VID,
#else
    .idVendor = CONFIG_TINYUSB_DESC_CUSTOM_VID,
#endif
    .idProduct = 0x4002,
    .bcdDevice = CONFIG_TINYUSB_DESC_BCD_DEVICE,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

static void handleTinyusbEvent(tinyusb_event_t* event, void* arg) {
    if (!event || !arg) {
        return;
    }
    UsbMscDevice* self = static_cast<UsbMscDevice*>(arg);
    if (event->id == TINYUSB_EVENT_ATTACHED) {
        self->notifyConnection(true);
    } else if (event->id == TINYUSB_EVENT_DETACHED) {
        self->notifyConnection(false);
    }
}

UsbMscDevice& UsbMscDevice::getInstance() {
    static UsbMscDevice instance;
    return instance;
}

void UsbMscDevice::notifyConnection(bool connected) {
    updateConnection(connected);
}

void UsbMscDevice::updateConnection(bool connected) {
    if (connected_.load() == connected) {
        return;
    }
    connected_.store(connected);
    if (status_cb_) {
        status_cb_(connected);
    }
}

esp_err_t UsbMscDevice::start(void* card) {
    if (active_.load()) {
        return ESP_OK;
    }
    if (!card) {
        ESP_LOGE(TAG, "No SD card handle provided");
        return ESP_ERR_INVALID_ARG;
    }
    auto* sd_card = static_cast<sdmmc_card_t*>(card);

    if (keyboard_host_is_running()) {
        keyboard_host_stop();
        keyboard_was_running_ = true;
        vTaskDelay(pdMS_TO_TICKS(50));
    } else {
        keyboard_was_running_ = false;
    }

    // Ensure HID device stack is not active before starting MSC.
    deinitUsbHidDevice();

    tinyusb_msc_driver_config_t msc_cfg = {};
    msc_cfg.user_flags.auto_mount_off = 1;
    msc_cfg.callback = nullptr;
    msc_cfg.callback_arg = nullptr;
    esp_err_t ret = tinyusb_msc_install_driver(&msc_cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "MSC driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    tinyusb_msc_storage_config_t storage_cfg = {};
    storage_cfg.medium.card = sd_card;
    storage_cfg.mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB;
    storage_cfg.fat_fs.base_path = const_cast<char*>("/sdcard");
    storage_cfg.fat_fs.config.max_files = 5;
    storage_cfg.fat_fs.config.format_if_mount_failed = false;
    storage_cfg.fat_fs.do_not_format = true;
    storage_cfg.fat_fs.format_flags = 0;

    tinyusb_msc_storage_handle_t handle = nullptr;
    ret = tinyusb_msc_new_storage_sdmmc(&storage_cfg, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MSC storage init failed: %s", esp_err_to_name(ret));
        tinyusb_msc_uninstall_driver();
        return ret;
    }
    storage_handle_ = handle;

    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG(handleTinyusbEvent, this);
    tusb_cfg.descriptor.device = &kMscDeviceDescriptor;

    ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB driver install failed: %s", esp_err_to_name(ret));
        tinyusb_msc_delete_storage(reinterpret_cast<tinyusb_msc_storage_handle_t>(storage_handle_));
        storage_handle_ = nullptr;
        tinyusb_msc_uninstall_driver();
        return ret;
    }

    active_.store(true);
    updateConnection(false);

    ESP_LOGI(TAG, "USB MSC device active");
    return ESP_OK;
}

esp_err_t UsbMscDevice::stop() {
    if (!active_.load()) {
        updateConnection(false);
        return ESP_OK;
    }

    active_.store(false);
    updateConnection(false);

    esp_err_t ret = tinyusb_driver_uninstall();
    if (ret != ESP_OK && ret != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "TinyUSB driver uninstall failed: %s", esp_err_to_name(ret));
    }

    if (storage_handle_) {
        esp_err_t storage_ret = tinyusb_msc_delete_storage(reinterpret_cast<tinyusb_msc_storage_handle_t>(storage_handle_));
        if (storage_ret != ESP_OK) {
            ESP_LOGW(TAG, "MSC storage delete failed: %s", esp_err_to_name(storage_ret));
        }
        storage_handle_ = nullptr;
    }

    esp_err_t msc_ret = tinyusb_msc_uninstall_driver();
    if (msc_ret != ESP_OK && msc_ret != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "MSC driver uninstall failed: %s", esp_err_to_name(msc_ret));
    }

    if (keyboard_was_running_) {
        keyboard_host_start();
        keyboard_was_running_ = false;
    }

    return (ret == ESP_OK) ? msc_ret : ret;
}
