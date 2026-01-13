#pragma once

#include <lvgl.h>
#include <esp_err.h>

// Unified touch controller with auto-detection
// Probes for GT911 (0x14) and ST7123 (0x55) touch controllers
// Returns singleton LVGL indev callback

namespace TouchController {

// Initialize touch controller with auto-detection
// Tries GT911 first, falls back to ST7123
// width/height: display dimensions for coordinate mapping
esp_err_t init(int width, int height);

// Get LVGL indev read callback
lv_indev_read_cb_t getReadCallback();

// Check if touch is initialized
bool isInitialized();

// Get the detected touch controller name (for debugging)
const char* getControllerName();

} // namespace TouchController
