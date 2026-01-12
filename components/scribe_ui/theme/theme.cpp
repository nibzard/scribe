#include "theme.h"
#include <esp_log.h>
#include <lvgl.h>

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

// Apply theme to default LVGL display
void applyTheme(bool dark) {
    current_dark = dark;
    const Colors& colors = dark ? dark_colors : light_colors;

    // Apply to default display
    lv_display_t* disp = lv_display_get_default();
    if (disp) {
        // Set default theme for this display
        lv_theme_t* theme = lv_theme_get_default();

        // Create a simple theme with our colors
        static lv_theme_t scribe_theme;
        lv_theme_t* theme_ptr = &scribe_theme;

        // Apply color scheme
        lv_color_t color_palette[] = {
            colors.bg,           // LV_PALETTE_MAIN (primary)
            colors.accent,       // LV_PALETTE_SECONDARY
            colors.selection,    // LV_PALETTE_TERTIARY
            colors.text,         // LV_PALETTE_QUATERNARY
            colors.text_secondary // LV_PALETTE_QUINT
        };

        // Apply theme to display
        for (int i = 0; i < 5; i++) {
            lv_palette_set(LV_PALETTE_MAIN + i, color_palette[i]);
        }

        ESP_LOGI(TAG, "Applied %s theme to display", dark ? "dark" : "light");
    } else {
        ESP_LOGW(TAG, "No default display found for theme application");
    }

    // Apply to default screen (background color)
    lv_obj_t* scr = lv_screen_active();
    if (scr) {
        lv_obj_set_style_bg_color(scr, colors.bg, 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(scr, colors.text, 0);
    }
}

const Colors& getColors() {
    return current_dark ? dark_colors : light_colors;
}

bool isDark() {
    return current_dark;
}

// Get font based on size setting
const lv_font_t* getFont(int size_setting) {
    switch (size_setting) {
        case 0:  // Small
            return &lv_font_montserrat_14;
        case 2:  // Large
            return &lv_font_montserrat_20;
        case 1:  // Medium (default)
        default:
            return &lv_font_montserrat_16;
    }
}

} // namespace Theme
