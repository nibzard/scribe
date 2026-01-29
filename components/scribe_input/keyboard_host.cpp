#include "keyboard_host.h"
#include "key_event.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "SCRIBE_KBD_HOST";

KeyboardHost& KeyboardHost::getInstance() {
    static KeyboardHost instance;
    return instance;
}

esp_err_t KeyboardHost::init() {
#if !SCRIBE_HAS_USB_HID
    ESP_LOGW(TAG, "USB HID host not available in this IDF build");
    return ESP_ERR_NOT_SUPPORTED;
#else
    ESP_LOGI(TAG, "Initializing USB HID host keyboard...");

    // Create semaphore for task synchronization
    task_semaphore_ = xSemaphoreCreateBinary();
    if (!task_semaphore_) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return ESP_ERR_NO_MEM;
    }

    // Initialize USB host library
    usb_host_config_t host_config = {
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB host: %s", esp_err_to_name(ret));
        vSemaphoreDelete(task_semaphore_);
        return ret;
    }

    hid_host_driver_config_t hid_cfg = {
        .create_background_task = false,
        .task_priority = 0,
        .stack_size = 0,
        .core_id = tskNO_AFFINITY,
        .callback = hidDriverCallback,
        .callback_arg = this,
    };
    ret = hid_host_install(&hid_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install HID host: %s", esp_err_to_name(ret));
        usb_host_uninstall();
        vSemaphoreDelete(task_semaphore_);
        return ret;
    }

    ESP_LOGI(TAG, "USB HID host initialized");
    return ESP_OK;
#endif
}

esp_err_t KeyboardHost::start() {
#if !SCRIBE_HAS_USB_HID
    ESP_LOGW(TAG, "Keyboard host not supported");
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (running_.load()) return ESP_OK;
    running_.store(true);

    // Create keyboard host task
    BaseType_t ret = xTaskCreate(keyboardHostTask, "kbd_host", 4096, this, 5, nullptr);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create keyboard host task");
        running_.store(false);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Keyboard host started");
    return ESP_OK;
#endif
}

esp_err_t KeyboardHost::stop() {
#if !SCRIBE_HAS_USB_HID
    return ESP_ERR_NOT_SUPPORTED;
#else
    running_.store(false);

    // Close device if open
    closeKeyboardDevice();

    // Signal task to wake up
    if (task_semaphore_) {
        xSemaphoreGive(task_semaphore_);
    }

    ESP_LOGI(TAG, "Keyboard host stopped");
    return ESP_OK;
#endif
}

esp_err_t KeyboardHost::openKeyboardDevice(hid_host_device_handle_t device_handle) {
#if !SCRIBE_HAS_USB_HID
    (void)device_handle;
    return ESP_ERR_NOT_SUPPORTED;
#else
    hid_host_dev_params_t dev_params = {};
    esp_err_t ret = hid_host_device_get_params(device_handle, &dev_params);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get HID device params: %s", esp_err_to_name(ret));
        return ret;
    }

    if (dev_params.proto != HID_PROTOCOL_KEYBOARD) {
        ESP_LOGI(TAG, "Ignoring HID device (proto=%u, subclass=%u)",
                 dev_params.proto, dev_params.sub_class);
        return ESP_OK;
    }

    hid_host_dev_info_t dev_info = {};
    if (hid_host_get_device_info(device_handle, &dev_info) == ESP_OK) {
        ESP_LOGI(TAG, "HID device VID=0x%04x PID=0x%04x", dev_info.VID, dev_info.PID);
    }

    hid_host_device_config_t dev_config = {
        .callback = hidHostCallback,
        .callback_arg = this,
    };

    ret = hid_host_device_open(device_handle, &dev_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HID device: %s", esp_err_to_name(ret));
        return ret;
    }

    hid_device_handle_ = device_handle;
    device_subclass_ = dev_params.sub_class;
    device_proto_ = dev_params.proto;
    report_len_warned_ = false;
    connected_.store(true);

    if (dev_params.sub_class == HID_SUBCLASS_BOOT_INTERFACE) {
        ret = hid_class_request_set_protocol(device_handle, HID_REPORT_PROTOCOL_BOOT);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to set boot protocol: %s", esp_err_to_name(ret));
        }
        ret = hid_class_request_set_idle(device_handle, 0, 0);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to set idle: %s", esp_err_to_name(ret));
        }
    }

    ret = hid_host_device_start(device_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to start HID device: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Keyboard device opened (addr=%u iface=%u subclass=%u proto=%u)",
             dev_params.addr, dev_params.iface_num, dev_params.sub_class, dev_params.proto);
    return ESP_OK;
#endif
}

esp_err_t KeyboardHost::closeKeyboardDevice() {
#if SCRIBE_HAS_USB_HID
    if (hid_device_handle_) {
        hid_host_device_close(hid_device_handle_);
    }
#endif
    hid_device_handle_ = nullptr;
    device_subclass_ = 0;
    device_proto_ = 0;
    report_len_warned_ = false;
    connected_.store(false);
    return ESP_OK;
}

void KeyboardHost::hidHostCallback(hid_host_device_handle_t hid_device_handle,
                                   const hid_host_interface_event_t event,
                                   void* arg) {
#if !SCRIBE_HAS_USB_HID
    (void)hid_device_handle;
    (void)event;
    (void)arg;
#else
    KeyboardHost* host = static_cast<KeyboardHost*>(arg);

    switch (event) {
        case HID_HOST_INTERFACE_EVENT_INPUT_REPORT: {
            if (host->device_proto_ != HID_PROTOCOL_KEYBOARD) {
                break;
            }
            uint8_t report[64] = {0};
            size_t report_len = 0;
            esp_err_t ret = hid_host_device_get_raw_input_report_data(
                hid_device_handle, report, sizeof(report), &report_len);
            if (ret == ESP_OK && report_len > 0) {
                host->parseHIDReport(report, report_len);
            }
            break;
        }
        case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HID keyboard disconnected");
            host->closeKeyboardDevice();
            break;
        case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
            ESP_LOGW(TAG, "HID transfer error");
            break;
        default:
            break;
    }
#endif
}

void KeyboardHost::parseHIDReport(const uint8_t* report, size_t len) {
    if (len < 8) {
        if (!report_len_warned_) {
            ESP_LOGW(TAG, "HID report length %u < 8; ignoring", static_cast<unsigned>(len));
            report_len_warned_ = true;
        }
        return;
    }

    // Standard boot protocol keyboard report:
    // Byte 0: modifier bits (Ctrl, Shift, Alt, GUI)
    // Byte 1: reserved
    // Bytes 2-7: up to 6 simultaneous keycodes

    static uint8_t prev_keys[6] = {0};

    uint8_t modifiers = report[0];
    const uint8_t* keys = &report[2];

    // Send key release events for keys that are no longer pressed
    for (int i = 0; i < 6; i++) {
        if (prev_keys[i] != 0) {
            bool still_pressed = false;
            for (int j = 0; j < 6; j++) {
                if (keys[j] == prev_keys[i]) {
                    still_pressed = true;
                    break;
                }
            }

            if (!still_pressed) {
                KeyEvent event;
                event.key = mapHIDUsageToKey(prev_keys[i]);
                event.pressed = false;
                event.shift = (modifiers & 0x22) != 0;  // Left or Right Shift
                event.ctrl = (modifiers & 0x11) != 0;   // Left or Right Ctrl
                event.alt = (modifiers & 0x44) != 0;    // Left or Right Alt
                event.meta = (modifiers & 0x88) != 0;   // Left or Right GUI
                event.char_code = 0;

                if (callback_) {
                    callback_(event);
                }
            }
        }
    }

    // Send key press events for new keys
    for (int i = 0; i < 6; i++) {
        if (keys[i] == 0) continue;

        bool was_pressed = false;
        for (int j = 0; j < 6; j++) {
            if (prev_keys[j] == keys[i]) {
                was_pressed = true;
                break;
            }
        }

        if (!was_pressed) {
            KeyEvent event;
            event.key = mapHIDUsageToKey(keys[i]);
            event.pressed = true;
            event.shift = (modifiers & 0x22) != 0;  // Left or Right Shift
            event.ctrl = (modifiers & 0x11) != 0;   // Left or Right Ctrl
            event.alt = (modifiers & 0x44) != 0;    // Left or Right Alt
            event.meta = (modifiers & 0x88) != 0;   // Left or Right GUI
            event.char_code = mapKeyToChar(event.key, event.shift);

            if (callback_) {
                callback_(event);
            }
        }
    }

    // Store current state
    memcpy(prev_keys, keys, 6);
}

void KeyboardHost::hidDriverCallback(hid_host_device_handle_t hid_device_handle,
                                     const hid_host_driver_event_t event,
                                     void* arg) {
#if !SCRIBE_HAS_USB_HID
    (void)hid_device_handle;
    (void)event;
    (void)arg;
#else
    KeyboardHost* host = static_cast<KeyboardHost*>(arg);
    if (event == HID_HOST_DRIVER_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "HID device connected");
        host->openKeyboardDevice(hid_device_handle);
    }
#endif
}

void KeyboardHost::keyboardHostTask(void* arg) {
    KeyboardHost* host = static_cast<KeyboardHost*>(arg);

    ESP_LOGI(TAG, "Keyboard host task running");

    while (host->running_.load()) {
        uint32_t event_flags = 0;
        usb_host_lib_handle_events(pdMS_TO_TICKS(10), &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI(TAG, "USB host: all devices freed");
        }
        hid_host_handle_events(pdMS_TO_TICKS(10));

        // Wait for semaphore with timeout
        xSemaphoreTake(host->task_semaphore_, pdMS_TO_TICKS(10));
        xSemaphoreGive(host->task_semaphore_);
    }

    vTaskDelete(nullptr);
}
