#include "keyboard_host.h"
#include "key_event.h"
#include <esp_log.h>
#include <usb/usb_host.h>
#include <usb/hid_host.h>
#include <freertos/semphr.h>
#include <cstring>

static const char* TAG = "SCRIBE_KBD_HOST";

// HID keyboard task
static void keyboard_host_task(void* arg);

KeyboardHost& KeyboardHost::getInstance() {
    static KeyboardHost instance;
    return instance;
}

esp_err_t KeyboardHost::init() {
    ESP_LOGI(TAG, "Initializing USB HID host keyboard...");

    // Initialize USB host library
    usb_host_config_t host_config = {
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB host: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "USB HID host initialized");
    return ESP_OK;
}

esp_err_t KeyboardHost::start() {
    if (running_) return ESP_OK;
    running_ = true;

    // Create keyboard host task
    xTaskCreate(keyboard_host_task, "kbd_host", 4096, this, 4, nullptr);

    ESP_LOGI(TAG, "Keyboard host started");
    return ESP_OK;
}

esp_err_t KeyboardHost::stop() {
    running_ = false;
    ESP_LOGI(TAG, "Keyboard host stopped");
    return ESP_OK;
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
        case USB_HOST_EVENT_DEVICE_ARRIVED:
            ESP_LOGI(TAG, "USB device arrived");
            // TODO: Check if HID keyboard and open it
            break;
        case USB_HOST_EVENT_DEVICE_REMOVED:
            ESP_LOGI(TAG, "USB device removed");
            host->connected_ = false;
            break;
        default:
            break;
    }
}

// HID keyboard callback
static void hid_host_callback(hid_host_device_handle_t hid_device, const hid_host_event_t event, void* arg) {
    KeyboardHost* host = static_cast<KeyboardHost*>(arg);

    switch (event) {
        case HID_HOST_EVENT_INPUT_REPORT:
            // TODO: Get input report and parse
            break;
        case HID_HOST_EVENT_DEVICE_DISCONNECTION:
            ESP_LOGI("SCRIBE_KBD_HOST", "HID device disconnected");
            host->connected_ = false;
            break;
        default:
            break;
    }
}

// Keyboard host task
static void keyboard_host_task(void* arg) {
    KeyboardHost* host = static_cast<KeyboardHost*>(arg);

    while (host->running_) {
        // Process USB host events
        usb_host_task_handle_events();

        // TODO: Handle HID device opening and data transfer
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(nullptr);
}
