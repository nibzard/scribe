#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
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
    BATTERY_LOW,        // Low battery warning
};

struct Event {
    EventType type;
    KeyEvent key_event{};
    void* data = nullptr;
    int int_param = 0;
};

// Global queue handle (set in app_main)
extern QueueHandle_t g_event_queue;
