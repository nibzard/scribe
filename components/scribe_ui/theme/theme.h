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

// Editor font sizes (pixels)
constexpr int FONT_SIZE_MIN = 14;
constexpr int FONT_SIZE_MAX = 240;
constexpr int FONT_SIZE_STEP = 2;
constexpr int FONT_SIZE_DEFAULT = 16;

// UI scale (percent). Current UI is calibrated at 50%.
constexpr int UI_SCALE_MIN = 50;
constexpr int UI_SCALE_MAX = 150;
constexpr int UI_SCALE_DEFAULT = 50;
constexpr int UI_SCALE_BASE = 50;

enum class UiFontRole {
    Small,
    Body,
    Title,
    Display
};

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

// UI scale helpers
void setUiScalePercent(int percent);
int getUiScalePercent();
int scalePx(int base_px);
int fitWidth(int base_px, int margin_px);
int fitHeight(int base_px, int margin_px);

// Font helpers
const char* getDefaultEditorFontId();
const char* getEditorFontLabelKey(const char* id);
const char* getNextEditorFontId(const char* current_id, int delta);
size_t getEditorFontCount();
const char* getEditorFontIdByIndex(size_t index);
int getEditorFontIndex(const char* id);
const lv_font_t* getEditorFont(const char* font_id, int size_px);
const lv_font_t* getUIFont(UiFontRole role);
int clampEditorFontSize(int size_px);

} // namespace Theme
