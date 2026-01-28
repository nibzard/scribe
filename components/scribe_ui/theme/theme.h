#pragma once

#include <lvgl.h>
#include <cstddef>

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
    lv_color_t success;
    lv_color_t warning;
    lv_color_t error;
    lv_color_t info;
};

// Theme descriptor
struct ThemeDefinition {
    const char* id;
    const char* label_key;
    bool dark;
    Colors colors;
    lv_color_t primary;
    lv_color_t secondary;
};

// Font sizes (pixels)
constexpr int FONT_SIZE_MIN = 12;
constexpr int FONT_SIZE_MAX = 28;
constexpr int FONT_SIZE_STEP = 2;
constexpr int FONT_SIZE_SMALL = 12;
constexpr int FONT_SIZE_MEDIUM = 16;
constexpr int FONT_SIZE_LARGE = 22;
constexpr int FONT_SIZE_DEFAULT = 16;

// Theme registry
size_t getThemeCount();
const ThemeDefinition& getThemeByIndex(size_t index);
const ThemeDefinition* findTheme(const char* id);
const char* getThemeLabelKey(const char* id);
const char* getThemeIdByIndex(size_t index);
int getThemeIndex(const char* id);
const char* getNextThemeId(const char* current_id, int delta);
const char* getDefaultThemeId();
const char* getCurrentThemeId();

// Apply theme to display
void applyTheme(const char* theme_id = nullptr);

// Apply base screen style using current theme colors
void applyScreenStyle(lv_obj_t* screen);

// Get current colors
const Colors& getColors();

// Check if dark theme is active
bool isDark();

// Get font based on size setting (pixel size)
const lv_font_t* getFont(int size_setting);

} // namespace Theme
