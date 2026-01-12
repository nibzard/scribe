#pragma once

#include <lvgl.h>

// Theme definitions for Scribe
// Light and dark themes optimized for distraction-free writing

namespace Theme {

// Color palette
struct Colors {
    lv_color_t bg;
    lv_color_t fg;
    lv_color_t text;
    lv_color_t text_secondary;
    lv_color_t accent;
    lv_color_t selection;
    lv_color_t border;
    lv_color_t shadow;
};

// Font sizes
constexpr int FONT_SIZE_SMALL = 14;
constexpr int FONT_SIZE_MEDIUM = 16;
constexpr int FONT_SIZE_LARGE = 20;

// Apply theme to display
void applyTheme(bool dark = false);

// Get current colors
const Colors& getColors();

// Check if dark theme is active
bool isDark();

// Get font based on size setting (0=small, 1=medium, 2=large)
const lv_font_t* getFont(int size_setting);

} // namespace Theme
