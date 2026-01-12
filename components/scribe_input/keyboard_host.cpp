#include "keyboard_host.h"
#include "key_event.h"
#include <esp_log.h>
#include <usb/usb_host.h>
#include <usb/hid_host.h>
#include <cstring>

static const char* TAG = "SCRIBE_KBD_HOST";

// HID report descriptor for boot protocol keyboard (simplified)
static const uint8_t keyboard_hid_report_desc[] = {
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x06,  // Usage (Keyboard)
    0xA1, 0x01,  // Collection (Application)
    0x05, 0x07,  // Usage Page (Keyboard)
    0x19, 0xE0,  // Usage Minimum (224)
    0x29, 0xE7,  // Usage Maximum (231)
    0x15, 0x00,  // Logical Minimum (0)
    0x25, 0x01,  // Logical Maximum (1)
    0x75, 0x01,  // Report Size (1)
    0x95, 0x08,  // Report Count (8)
    0x81, 0x02,  // Input (Data, Var, Abs)
    0x95, 0x01,  // Report Count (1)
    0x75, 0x08,  // Report Size (8)
    0x81, 0x03,  // Input (Cnst, Var, Abs)
    0x95, 0x06,  // Report Count (6)
    0x75, 0x08,  // Report Size (8)
    0x15, 0x00,  // Logical Minimum (0)
    0x25, 0x65,  // Logical Maximum (101)
    0x05, 0x07,  // Usage Page (Keyboard)
    0x19, 0x00,  // Usage Minimum (0)
    0x29, 0x65,  // Usage Maximum (101)
    0x81, 0x00,  // Input (Data, Ary, Abs)
    0xC0         // End Collection
};

KeyboardHost& KeyboardHost::getInstance() {
    static KeyboardHost instance;
    return instance;
}

esp_err_t KeyboardHost::init() {
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

    ESP_LOGI(TAG, "USB HID host initialized");
    return ESP_OK;
}

esp_err_t KeyboardHost::start() {
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
}

esp_err_t KeyboardHost::stop() {
    running_.store(false);

    // Close device if open
    closeKeyboardDevice();

    // Signal task to wake up
    if (task_semaphore_) {
        xSemaphoreGive(task_semaphore_);
    }

    ESP_LOGI(TAG, "Keyboard host stopped");
    return ESP_OK;
}

esp_err_t KeyboardHost::openKeyboardDevice(uint8_t address) {
    hid_host_device_config_t dev_config = {
        .callback = hidHostCallback,
        .callback_arg = this,
    };

    esp_err_t ret = hid_host_device_open(address, keyboard_hid_report_desc,
                                         sizeof(keyboard_hid_report_desc), &dev_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HID device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Get the device handle
    hid_host_device_handle_t handle;
    ret = hid_host_device_get_handle(address, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get HID device handle: %s", esp_err_to_name(ret));
        hid_host_device_close(handle);
        return ret;
    }

    hid_device_handle_ = handle;
    connected_.store(true);

    // Subscribe to input reports
    ret = hid_host_device_subscribe_events(handle, HID_HOST_EVENT_INPUT_REPORT);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to subscribe to input reports: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Keyboard device opened at address %u", address);
    return ESP_OK;
}

esp_err_t KeyboardHost::closeKeyboardDevice() {
    if (hid_device_handle_) {
        hid_host_device_close(hid_device_handle_);
        hid_device_handle_ = nullptr;
    }
    connected_.store(false);
    return ESP_OK;
}

void KeyboardHost::hidHostCallback(hid_host_device_handle_t hid_device_handle,
                                   const hid_host_event_t event, void* arg) {
    KeyboardHost* host = static_cast<KeyboardHost*>(arg);

    switch (event) {
        case HID_HOST_EVENT_INPUT_REPORT: {
            // Get the input report
            uint8_t report[8] = {0};
            size_t report_len = sizeof(report);

            esp_err_t ret = hid_host_device_get_report(hid_device_handle, report, &report_len);
            if (ret == ESP_OK) {
                host->parseHIDReport(report, report_len);
            }
            break;
        }
        case HID_HOST_EVENT_DEVICE_DISCONNECTION:
            ESP_LOGI(TAG, "HID keyboard disconnected");
            host->closeKeyboardDevice();
            break;
        default:
            break;
    }
}

void KeyboardHost::parseHIDReport(const uint8_t* report, size_t len) {
    if (len < 8) return;

    // Standard boot protocol keyboard report:
    // Byte 0: modifier bits (Ctrl, Shift, Alt, GUI)
    // Byte 1: reserved
    // Bytes 2-7: up to 6 simultaneous keycodes

    static uint8_t prev_keys[6] = {0};
    static uint8_t prev_modifiers = 0;

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
            event.char_code = mapKeyToChar(event.key, event.shift);

            if (callback_) {
                callback_(event);
            }
        }
    }

    // Store current state
    memcpy(prev_keys, keys, 6);
    prev_modifiers = modifiers;
}

void KeyboardHost::usbEventCallback(const usb_host_event_msg_t* event_msg, void* arg) {
    KeyboardHost* host = static_cast<KeyboardHost*>(arg);

    switch (event_msg->event) {
        case USB_HOST_EVENT_DEVICE_ARRIVED: {
            ESP_LOGI(TAG, "USB device arrived");

            // Try to open as HID keyboard
            uint8_t address = event_msg->device.address;
            host->openKeyboardDevice(address);
            break;
        }
        case USB_HOST_EVENT_DEVICE_REMOVED:
            ESP_LOGI(TAG, "USB device removed");
            host->closeKeyboardDevice();
            break;
        default:
            break;
    }
}

void KeyboardHost::keyboardHostTask(void* arg) {
    KeyboardHost* host = static_cast<KeyboardHost*>(arg);

    // Register USB event callback
    usb_host_install_client_callbacks(
        usb_host_client_event_callbacks_t{
            .event_cb = usbEventCallback,
        },
        host
    );

    ESP_LOGI(TAG, "Keyboard host task running");

    while (host->running_.load()) {
        // Process USB host events
        usb_host_task_handle_events();

        // Wait for semaphore with timeout
        xSemaphoreTake(host->task_semaphore_, pdMS_TO_TICKS(10));
        xSemaphoreGive(host->task_semaphore_);
    }

    // Unregister USB event callback
    usb_host_uninstall_client();

    vTaskDelete(nullptr);
}

void KeyboardHost::processEvents() {
    // This is called from the task loop
    // Events are processed through callbacks
}
