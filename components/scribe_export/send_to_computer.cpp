#include "send_to_computer.h"
#include "../scribe_input/keymap.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>

static const char* TAG = "SCRIBE_SEND_TO_COMPUTER";

// TinyUSB HID task
static void send_task(void* arg);

SendToComputer& SendToComputer::getInstance() {
    static SendToComputer instance;
    return instance;
}

esp_err_t SendToComputer::init() {
    ESP_LOGI(TAG, "Initializing Send to Computer (USB HID device mode)...");

    // TODO: Initialize TinyUSB device stack
    // TODO: Configure HID keyboard descriptor
    // TODO: Set up USB device mode (switch from host mode)

    ESP_LOGI(TAG, "Send to Computer initialized");
    return ESP_OK;
}

esp_err_t SendToComputer::start(const std::string& text, ExportProgressCallback callback) {
    if (running_) {
        ESP_LOGW(TAG, "Already sending");
        return ESP_ERR_INVALID_STATE;
    }

    text_to_send_ = text;
    send_pos_ = 0;
    progress_callback_ = callback;
    running_ = true;

    ESP_LOGI(TAG, "Starting to send %zu characters", text_to_send_.length());

    // Create background task for sending
    xTaskCreate(send_task, "send_task", 4096, this, 3, nullptr);

    return ESP_OK;
}

esp_err_t SendToComputer::stop() {
    if (!running_) return ESP_OK;

    running_ = false;
    ESP_LOGI(TAG, "Stopped sending at position %zu/%zu", send_pos_, text_to_send_.length());

    // TODO: Switch back to USB host mode for keyboard input

    return ESP_OK;
}

// Convert character to USB HID keycode(s)
// Returns number of keycodes used (1-2 for shifted chars)
static int charToHIDKeycodes(char c, uint8_t* keycodes, uint8_t* modifiers) {
    *modifiers = 0;

    // Handle common ASCII characters
    if (c >= 'a' && c <= 'z') {
        keycodes[0] = 0x04 + (c - 'a');  // A=0x04, B=0x05, etc.
        return 1;
    }

    if (c >= 'A' && c <= 'Z') {
        keycodes[0] = 0x04 + (c - 'A');
        *modifiers |= 0x02;  // Left Shift
        return 1;
    }

    if (c >= '1' && c <= '9') {
        keycodes[0] = 0x1E + (c - '1');
        return 1;
    }

    if (c == '0') {
        keycodes[0] = 0x27;
        return 1;
    }

    // Special characters
    switch (c) {
        case ' ': keycodes[0] = 0x2C; return 1;     // Space
        case '\n': keycodes[0] = 0x28; return 1;    // Enter
        case '\t': keycodes[0] = 0x2B; return 1;    // Tab
        case '-': keycodes[0] = 0x2D; return 1;
        case '_': keycodes[0] = 0x2D; *modifiers |= 0x02; return 1;
        case '=': keycodes[0] = 0x2E; return 1;
        case '+': keycodes[0] = 0x2E; *modifiers |= 0x02; return 1;
        case '[': keycodes[0] = 0x2F; return 1;
        case '{': keycodes[0] = 0x2F; *modifiers |= 0x02; return 1;
        case ']': keycodes[0] = 0x30; return 1;
        case '}': keycodes[0] = 0x30; *modifiers |= 0x02; return 1;
        case '\\': keycodes[0] = 0x31; return 1;
        case '|': keycodes[0] = 0x31; *modifiers |= 0x02; return 1;
        case ';': keycodes[0] = 0x33; return 1;
        case ':': keycodes[0] = 0x33; *modifiers |= 0x02; return 1;
        case '\'': keycodes[0] = 0x34; return 1;
        case '"': keycodes[0] = 0x34; *modifiers |= 0x02; return 1;
        case '`': keycodes[0] = 0x35; return 1;
        case '~': keycodes[0] = 0x35; *modifiers |= 0x02; return 1;
        case ',': keycodes[0] = 0x36; return 1;
        case '<': keycodes[0] = 0x36; *modifiers |= 0x02; return 1;
        case '.': keycodes[0] = 0x37; return 1;
        case '>': keycodes[0] = 0x37; *modifiers |= 0x02; return 1;
        case '/': keycodes[0] = 0x38; return 1;
        case '?': keycodes[0] = 0x38; *modifiers |= 0x02; return 1;
        case '!': keycodes[0] = 0x1E; *modifiers |= 0x02; return 1;  // Shift+1
        case '@': keycodes[0] = 0x1F; *modifiers |= 0x02; return 1;  // Shift+2
        case '#': keycodes[0] = 0x20; *modifiers |= 0x02; return 1;  // Shift+3
        case '$': keycodes[0] = 0x21; *modifiers |= 0x02; return 1;  // Shift+4
        case '%': keycodes[0] = 0x22; *modifiers |= 0x02; return 1;  // Shift+5
        case '^': keycodes[0] = 0x23; *modifiers |= 0x02; return 1;  // Shift+6
        case '&': keycodes[0] = 0x24; *modifiers |= 0x02; return 1;  // Shift+7
        case '*': keycodes[0] = 0x25; *modifiers |= 0x02; return 1;  // Shift+8
        case '(': keycodes[0] = 0x26; *modifiers |= 0x02; return 1;  // Shift+9
        case ')': keycodes[0] = 0x27; *modifiers |= 0x02; return 1;  // Shift+0
    }

    return 0;  // Unknown character
}

// Send task - types characters one by one
static void send_task(void* arg) {
    SendToComputer* sender = static_cast<SendToComputer*>(arg);

    while (sender->running_ && sender->send_pos_ < sender->text_to_send_.length()) {
        char c = sender->text_to_send_[sender->send_pos_];

        // Convert character to HID keycode
        uint8_t keycodes[6] = {0};
        uint8_t modifiers = 0;
        int key_count = charToHIDKeycodes(c, keycodes, &modifiers);

        if (key_count > 0) {
            // TODO: Send HID report via TinyUSB
            // Format: [modifiers, reserved, keycode1, keycode2, keycode3, keycode4, keycode5, keycode6]
            //
            // Example for TinyUSB:
            // tud_hid_report(0, report, 8);
            //
            // Then release by sending report with all zeros
        }

        sender->send_pos_++;

        // Call progress callback
        if (sender->progress_callback_) {
            sender->progress_callback_(sender->send_pos_, sender->text_to_send_.length());
        }

        // Rate limit: 80 chars/sec = ~12.5ms per character
        vTaskDelay(pdMS_TO_TICKS(1000 / SendToComputer::CHARS_PER_SEC));
    }

    sender->running_ = false;
    ESP_LOGI(TAG, "Finished sending %zu characters", sender->send_pos_);

    vTaskDelete(nullptr);
}
