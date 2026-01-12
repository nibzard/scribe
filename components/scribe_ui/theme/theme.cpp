#include "theme.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_THEME";

namespace Theme {

// Current theme state
static bool current_dark = false;

// Light theme colors
static constexpr Colors light_colors = {
    .bg = LV_COLOR_MAKE(0xFA, 0xFA, 0xFA),      // Off-white background
    .fg = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),      // White elements
    .text = LV_COLOR_MAKE(0x20, 0x20, 0x20),    // Dark text
    .text_secondary = LV_COLOR_MAKE(0x75, 0x75, 0x75),
    .accent = LV_COLOR_MAKE(0x42, 0xA5, 0xF5),  // Blue accent
    .selection = LV_COLOR_MAKE(0x64, 0xB5, 0xF6),
    .border = LV_COLOR_MAKE(0xE0, 0xE0, 0xE0),
    .shadow = LV_COLOR_MAKE(0x00, 0x00, 0x00),
};

// Dark theme colors
static constexpr Colors dark_colors = {
    .bg = LV_COLOR_MAKE(0x1E, 0x1E, 0x1E),      // Dark gray background
    .fg = LV_COLOR_MAKE(0x2D, 0x2D, 0x2D),      // Slightly lighter elements
    .text = LV_COLOR_MAKE(0xE0, 0xE0, 0xE0),    // Light text
    .text_secondary = LV_COLOR_MAKE(0x9E, 0x9E, 0x9E),
    .accent = LV_COLOR_MAKE(0x64, 0xB5, 0xF6),  // Blue accent
    .selection = LV_COLOR_MAKE(0x42, 0xA5, 0xF5),
    .border = LV_COLOR_MAKE(0x30, 0x30, 0x30),
    .shadow = LV_COLOR_MAKE(0x00, 0x00, 0x00),
};

void applyTheme(bool dark) {
    current_dark = dark;
    const Colors& colors = dark ? dark_colors : light_colors;

    // TODO: Apply theme to LVGL display
    ESP_LOGI(TAG, "Applied %s theme", dark ? "dark" : "light");
}

const Colors& getColors() {
    return current_dark ? dark_colors : light_colors;
}

} // namespace Theme
