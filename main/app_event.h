#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstdint>
#include <string>
#include "key_event.h"

// Event types for inter-task communication
enum class EventType : uint8_t {
    KEY_EVENT,          // Keyboard input
    MENU_OPEN,          // Open Esc menu
    MENU_CLOSE,         // Close menu
    PROJECT_OPEN,       // Open specific project
    PROJECT_NEW,        // Create new project
    PROJECT_SWITCH,     // Switch project
    FIND_OPEN,          // Open find bar
    EXPORT_START,       // Start export
    SAVE_REQUEST,       // Manual save (Ctrl+S)
    HUD_TOGGLE,         // Toggle HUD (F1/Space hold)
    SETTINGS_OPEN,      // Open settings
    HELP_OPEN,          // Open help
    SHOW_FIRST_RUN,     // Show first-run tip
    SHOW_RECOVERY,      // Show recovery dialog
    SLEEP_ENTER,        // Enter sleep
    POWER_OFF,          // Power off
    STORAGE_SAVE_DONE,  // Autosave complete
    STORAGE_ERROR,      // Storage failure
    STORAGE_MOUNT_FAILED, // SD mount failed (offer format)
    STORAGE_FORMAT_STATUS, // SD format progress/completion update
    USB_STORAGE_REQUEST, // Enter/exit USB storage mode
    USB_STORAGE_STATUS,  // USB attach/detach status (int_param: 1=connected)
    BATTERY_LOW,        // Low battery warning
};

struct StorageFormatStatus {
    bool done = false;
    bool success = false;
    std::string status;
    std::string detail;
};

struct Event {
    EventType type;
    KeyEvent key_event{};
    void* data = nullptr;
    int int_param = 0;
    uint64_t u64_param = 0;
};

// Global queue handle (set in app_main)
extern QueueHandle_t g_event_queue;
