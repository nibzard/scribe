#pragma once

#include <lvgl.h>
#include <esp_err.h>

// Display driver integration for Scribe
// Handles LVGL display setup, buffers, and flush callbacks

namespace DisplayDriver {

// Display configuration
struct DisplayConfig {
    int width;
    int height;
    int rotation;           // 0, 90, 180, 270 degrees
    bool double_buffer;     // Use double buffering
    int buffer_size;        // Buffer size in lines (0 = full screen)
    bool partial_refresh;   // Enable partial refresh (e-ink style)
};

// Initialize display driver
// Creates LVGL display, sets up buffers and flush callback
esp_err_t init(const DisplayConfig& config);

// Flush callback - called by LVGL when display needs updating
// Implement this for your specific hardware (MIPI-DSI, SPI, etc.)
void flushCallback(lv_display_t* display, const lv_area_t* area, uint8_t* px_map);

// Set rotation (update display dimensions)
esp_err_t setRotation(int rotation_degrees);

// Get current display dimensions
void getDimensions(int* width, int* height);

// Power control
esp_err_t setBacklight(int percent);  // 0-100
esp_err_t displayOn(bool on);

// Clean up display driver
void deinit();

// Get LVGL display handle (for custom operations)
lv_display_t* getDisplay();

} // namespace DisplayDriver
