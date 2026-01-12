#include "send_to_computer.h"
#include "../scribe_input/keymap.h"
#include <esp_log.h>
#include <cstring>

#ifdef CONFIG_TINYUSB_ENABLED
#include <tusb.h>
#endif

static const char* TAG = "SCRIBE_SEND_TO_COMPUTER";

SendToComputer& SendToComputer::getInstance() {
    static SendToComputer instance;
    return instance;
}

esp_err_t SendToComputer::init() {
    ESP_LOGI(TAG, "Initializing Send to Computer (USB HID device mode)...");

    // Create mutex for thread safety
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

#ifdef CONFIG_TINYUSB_ENABLED
    // TinyUSB is initialized by the USB stack
    ESP_LOGI(TAG, "TinyUSB support enabled");
#else
    ESP_LOGW(TAG, "TinyUSB not enabled - Send to Computer requires CONFIG_TINYUSB_ENABLED");
#endif

    ESP_LOGI(TAG, "Send to Computer initialized");
    return ESP_OK;
}

esp_err_t SendToComputer::switchToDeviceMode() {
    ESP_LOGI(TAG, "Switching to USB device mode...");

    // Note: On Tab5, this requires:
    // 1. Stop USB host (keyboard input)
    // 2. Start USB device (HID keyboard)
    // User may need to unplug keyboard or use dual-port configuration

#ifdef CONFIG_TINYUSB_ENABLED
    // TinyUSB device stack should already be initialized
    // Just need to ensure we're in device mode
    device_mode_active_.store(true);
    ESP_LOGI(TAG, "Device mode active");
    return ESP_OK;
#else
    ESP_LOGE(TAG, "TinyUSB not available - cannot switch to device mode");
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t SendToComputer::switchToHostMode() {
    ESP_LOGI(TAG, "Switching back to USB host mode...");

    device_mode_active_.store(false);

    // Reinitialize keyboard host
    // This would be called after export completes

    return ESP_OK;
}

SendToComputer::HIDKeycode SendToComputer::charToHIDKeycode(char c) const {
    HIDKeycode result = {0, 0};

    // Handle common ASCII characters
    if (c >= 'a' && c <= 'z') {
        result.keycode = 0x04 + (c - 'a');  // A=0x04, B=0x05, etc.
        return result;
    }

    if (c >= 'A' && c <= 'Z') {
        result.keycode = 0x04 + (c - 'A');
        result.modifiers = 0x02;  // Left Shift
        return result;
    }

    if (c >= '1' && c <= '9') {
        result.keycode = 0x1E + (c - '1');
        return result;
    }

    if (c == '0') {
        result.keycode = 0x27;
        return result;
    }

    // Special characters
    switch (c) {
        case ' ': result.keycode = 0x2C; break;     // Space
        case '\n': result.keycode = 0x28; break;    // Enter
        case '\t': result.keycode = 0x2B; break;    // Tab
        case '-': result.keycode = 0x2D; break;
        case '_': result.keycode = 0x2D; result.modifiers = 0x02; break;
        case '=': result.keycode = 0x2E; break;
        case '+': result.keycode = 0x2E; result.modifiers = 0x02; break;
        case '[': result.keycode = 0x2F; break;
        case '{': result.keycode = 0x2F; result.modifiers = 0x02; break;
        case ']': result.keycode = 0x30; break;
        case '}': result.keycode = 0x30; result.modifiers = 0x02; break;
        case '\\': result.keycode = 0x31; break;
        case '|': result.keycode = 0x31; result.modifiers = 0x02; break;
        case ';': result.keycode = 0x33; break;
        case ':': result.keycode = 0x33; result.modifiers = 0x02; break;
        case '\'': result.keycode = 0x34; break;
        case '"': result.keycode = 0x34; result.modifiers = 0x02; break;
        case '`': result.keycode = 0x35; break;
        case '~': result.keycode = 0x35; result.modifiers = 0x02; break;
        case ',': result.keycode = 0x36; break;
        case '<': result.keycode = 0x36; result.modifiers = 0x02; break;
        case '.': result.keycode = 0x37; break;
        case '>': result.keycode = 0x37; result.modifiers = 0x02; break;
        case '/': result.keycode = 0x38; break;
        case '?': result.keycode = 0x38; result.modifiers = 0x02; break;
        case '!': result.keycode = 0x1E; result.modifiers = 0x02; break;  // Shift+1
        case '@': result.keycode = 0x1F; result.modifiers = 0x02; break;  // Shift+2
        case '#': result.keycode = 0x20; result.modifiers = 0x02; break;  // Shift+3
        case '$': result.keycode = 0x21; result.modifiers = 0x02; break;  // Shift+4
        case '%': result.keycode = 0x22; result.modifiers = 0x02; break;  // Shift+5
        case '^': result.keycode = 0x23; result.modifiers = 0x02; break;  // Shift+6
        case '&': result.keycode = 0x24; result.modifiers = 0x02; break;  // Shift+7
        case '*': result.keycode = 0x25; result.modifiers = 0x02; break;  // Shift+8
        case '(': result.keycode = 0x26; result.modifiers = 0x02; break;  // Shift+9
        case ')': result.keycode = 0x27; result.modifiers = 0x02; break;  // Shift+0
        default:
            // Unknown character - skip
            result.keycode = 0;
            break;
    }

    return result;
}

esp_err_t SendToComputer::sendHIDReport(uint8_t keycode, uint8_t modifiers, bool press) {
#ifdef CONFIG_TINYUSB_ENABLED
    if (!tud_ready()) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t report[8] = {0};
    report[0] = modifiers;

    if (press && keycode > 0) {
        report[2] = keycode;
    }

    // Send HID report
    if (tud_hid_n_ready(0)) {
        tud_hid_n_report(0, 0, report, sizeof(report));
        return ESP_OK;
    }

    return ESP_ERR_INVALID_STATE;
#else
    // Simulated mode for testing without TinyUSB
    ESP_LOGV(TAG, "Simulated HID: keycode=0x%02X, modifiers=0x%02X, press=%d",
             keycode, modifiers, press);
    return ESP_OK;
#endif
}

esp_err_t SendToComputer::releaseAllKeys() {
    return sendHIDReport(0, 0, false);
}

esp_err_t SendToComputer::start(const std::string& text, ExportProgressCallback callback) {
    if (running_.load()) {
        ESP_LOGW(TAG, "Already sending");
        return ESP_ERR_INVALID_STATE;
    }

    // Switch to device mode
    esp_err_t ret = switchToDeviceMode();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to switch to device mode: %s", esp_err_to_name(ret));
        return ret;
    }

    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return ESP_ERR_INVALID_STATE;
    }

    text_to_send_ = text;
    send_pos_ = 0;
    progress_callback_ = callback;
    running_.store(true);

    xSemaphoreGive(mutex_);

    ESP_LOGI(TAG, "Starting to send %zu characters", text_to_send_.length());

    // Create background task for sending
    BaseType_t task_ret = xTaskCreate(sendTask, "send_task", 4096, this, 3, &send_task_handle_);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create send task");
        running_.store(false);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t SendToComputer::stop() {
    if (!running_.load()) return ESP_OK;

    ESP_LOGI(TAG, "Stopping send at position %zu/%zu", send_pos_, text_to_send_.length());

    running_.store(false);

    // Wait for task to finish
    if (send_task_handle_) {
        // Give task time to clean up
        vTaskDelay(pdMS_TO_TICKS(100));
        send_task_handle_ = nullptr;
    }

    // Release any pressed keys
    releaseAllKeys();

    // Switch back to host mode
    switchToHostMode();

    return ESP_OK;
}

void SendToComputer::sendTask(void* arg) {
    SendToComputer* sender = static_cast<SendToComputer*>(arg);

    while (sender->running_.load() && sender->send_pos_ < sender->text_to_send_.length()) {
        char c = sender->text_to_send_[sender->send_pos_];

        // Convert character to HID keycode
        HIDKeycode hid = sender->charToHIDKeycode(c);

        if (hid.keycode > 0) {
            // Press key
            sender->sendHIDReport(hid.keycode, hid.modifiers, true);

            // Brief hold for registration
            vTaskDelay(pdMS_TO_TICKS(10));

            // Release key
            sender->sendHIDReport(hid.keycode, hid.modifiers, false);

            // Brief delay between keystrokes
            vTaskDelay(pdMS_TO_TICKS(1000 / CHARS_PER_SEC - 10));
        } else {
            // Skip unknown character
            ESP_LOGW(TAG, "Skipping unknown character: 0x%02X", c);
        }

        sender->send_pos_++;

        // Call progress callback
        if (sender->progress_callback_) {
            sender->progress_callback_(sender->send_pos_, sender->text_to_send_.length());
        }
    }

    sender->running_.store(false);

    // Ensure all keys are released
    sender->releaseAllKeys();

    // Switch back to host mode
    sender->switchToHostMode();

    ESP_LOGI(TAG, "Finished sending %zu characters", sender->send_pos_);

    vTaskDelete(nullptr);
}
