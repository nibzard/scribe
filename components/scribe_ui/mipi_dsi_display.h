#pragma once

#include <lvgl.h>
#include <esp_err.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <cstddef>

// MIPI-DSI Display Driver for Scribe
// Supports common MIPI-DSI panels used in TFT displays

namespace MIPIDSI {

// Display panel types
enum class PanelType {
    ST7789,     // Common 240x320/240x240 TFT
    ILI9341,    // Common 240x320 TFT
    ILI9488,    // 320x480 TFT
    ST7796,     // 320x480 TFT
    CUSTOM      // User-defined panel
};

// Display configuration
struct DisplayConfig {
    PanelType panel;
    int width;
    int height;
    int fps;                    // Target frames per second
    bool use_spi;               // Use SPI instead of MIPI-DSI
    int spi_freq_mhz;           // SPI frequency in MHz
    gpio_num_t dc_pin;          // Data/Command select
    gpio_num_t reset_pin;       // Reset pin
    gpio_num_t cs_pin;          // Chip select (SPI only)
    gpio_num_t backlight_pin;   // Backlight control
    bool rgb565_byte_swap;      // Swap bytes for RGB565
};

// Color formats
enum class ColorFormat {
    RGB565 = 0,
    RGB666,
    RGB888
};

// Orientation
enum class Orientation {
    PORTRAIT = 0,
    LANDSCAPE = 1,
    PORTRAIT_INVERTED = 2,
    LANDSCAPE_INVERTED = 3
};

// Initialize MIPI-DSI display
esp_err_t init(const DisplayConfig& config);

// Set display orientation
esp_err_t setOrientation(Orientation orientation);

// Write to display (flush callback implementation)
void writePixels(const uint16_t* pixels, size_t len);

// Fill rectangle with color
void fillRect(int x1, int y1, int x2, int y2, uint16_t color);

// Clear display
void clear(uint16_t color);

// Power control
esp_err_t reset();
esp_err_t sleep(bool enter);
esp_err_t setBacklight(int percent);  // 0-100

// Get LVGL display handle
lv_display_t* getLVGLDisplay();

// Get current configuration
const DisplayConfig& getConfig();

// Clean up
void deinit();

} // namespace MIPIDSI
