#include "theme.h"
#include <esp_log.h>
#include <lvgl.h>
#include "themes/default/lv_theme_default.h"
#include <cstring>
#include <cstdlib>

static const char* TAG = "SCRIBE_THEME";

namespace Theme {

static constexpr const char* kDefaultThemeId = "dracula";

static const ThemeDefinition kThemes[] = {
    {
        .id = "scribe_light",
        .label_key = "settings.theme_scribe_light",
        .dark = false,
        .colors = {
            .bg = LV_COLOR_MAKE(0xFA, 0xFA, 0xFA),
            .fg = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
            .text = LV_COLOR_MAKE(0x20, 0x20, 0x20),
            .text_secondary = LV_COLOR_MAKE(0x75, 0x75, 0x75),
        .accent = LV_COLOR_MAKE(0x42, 0xA5, 0xF5),
        .selection = LV_COLOR_MAKE(0x64, 0xB5, 0xF6),
        .border = LV_COLOR_MAKE(0xE0, 0xE0, 0xE0),
        .shadow = LV_COLOR_MAKE(0x00, 0x00, 0x00),
        .success = LV_COLOR_MAKE(0x4C, 0xAF, 0x50),
        .warning = LV_COLOR_MAKE(0xFF, 0xC1, 0x07),
        .error = LV_COLOR_MAKE(0xF4, 0x43, 0x36),
        .info = LV_COLOR_MAKE(0x21, 0x96, 0xF3),
    },
        .primary = LV_COLOR_MAKE(0x42, 0xA5, 0xF5),
        .secondary = LV_COLOR_MAKE(0x64, 0xB5, 0xF6),
    },
    {
        .id = "scribe_dark",
        .label_key = "settings.theme_scribe_dark",
        .dark = true,
        .colors = {
            .bg = LV_COLOR_MAKE(0x1E, 0x1E, 0x1E),
            .fg = LV_COLOR_MAKE(0x2D, 0x2D, 0x2D),
            .text = LV_COLOR_MAKE(0xE0, 0xE0, 0xE0),
            .text_secondary = LV_COLOR_MAKE(0x9E, 0x9E, 0x9E),
        .accent = LV_COLOR_MAKE(0x64, 0xB5, 0xF6),
        .selection = LV_COLOR_MAKE(0x42, 0xA5, 0xF5),
        .border = LV_COLOR_MAKE(0x30, 0x30, 0x30),
        .shadow = LV_COLOR_MAKE(0x00, 0x00, 0x00),
        .success = LV_COLOR_MAKE(0x4C, 0xAF, 0x50),
        .warning = LV_COLOR_MAKE(0xFF, 0xC1, 0x07),
        .error = LV_COLOR_MAKE(0xF4, 0x43, 0x36),
        .info = LV_COLOR_MAKE(0x21, 0x96, 0xF3),
    },
        .primary = LV_COLOR_MAKE(0x64, 0xB5, 0xF6),
        .secondary = LV_COLOR_MAKE(0x42, 0xA5, 0xF5),
    },
    {
        .id = "dracula",
        .label_key = "settings.theme_dracula",
        .dark = true,
        .colors = {
            .bg = LV_COLOR_MAKE(0x28, 0x2A, 0x36),
            .fg = LV_COLOR_MAKE(0x44, 0x47, 0x5A),
            .text = LV_COLOR_MAKE(0xF8, 0xF8, 0xF2),
            .text_secondary = LV_COLOR_MAKE(0x62, 0x72, 0xA4),
        .accent = LV_COLOR_MAKE(0xBD, 0x93, 0xF9),
        .selection = LV_COLOR_MAKE(0x44, 0x47, 0x5A),
        .border = LV_COLOR_MAKE(0x3B, 0x3D, 0x4A),
        .shadow = LV_COLOR_MAKE(0x00, 0x00, 0x00),
        .success = LV_COLOR_MAKE(0x50, 0xFA, 0x7B),
        .warning = LV_COLOR_MAKE(0xFF, 0xB8, 0x6C),
        .error = LV_COLOR_MAKE(0xFF, 0x55, 0x55),
        .info = LV_COLOR_MAKE(0x8B, 0xE9, 0xFD),
    },
        .primary = LV_COLOR_MAKE(0xBD, 0x93, 0xF9),
        .secondary = LV_COLOR_MAKE(0x8B, 0xE9, 0xFD),
    },
    {
        .id = "catppuccin_latte",
        .label_key = "settings.theme_catppuccin_latte",
        .dark = false,
        .colors = {
            .bg = LV_COLOR_MAKE(0xEF, 0xF1, 0xF5),
            .fg = LV_COLOR_MAKE(0xCC, 0xD0, 0xDA),
            .text = LV_COLOR_MAKE(0x4C, 0x4F, 0x69),
            .text_secondary = LV_COLOR_MAKE(0x6C, 0x6F, 0x85),
        .accent = LV_COLOR_MAKE(0x88, 0x39, 0xEF),
        .selection = LV_COLOR_MAKE(0xBC, 0xC0, 0xCC),
        .border = LV_COLOR_MAKE(0xDC, 0xE0, 0xE8),
        .shadow = LV_COLOR_MAKE(0x00, 0x00, 0x00),
        .success = LV_COLOR_MAKE(0x40, 0xA0, 0x2B),
        .warning = LV_COLOR_MAKE(0xFE, 0x64, 0x0B),
        .error = LV_COLOR_MAKE(0xD2, 0x0F, 0x39),
        .info = LV_COLOR_MAKE(0x1E, 0x66, 0xF5),
    },
        .primary = LV_COLOR_MAKE(0x88, 0x39, 0xEF),
        .secondary = LV_COLOR_MAKE(0x1E, 0x66, 0xF5),
    },
    {
        .id = "catppuccin_mocha",
        .label_key = "settings.theme_catppuccin_mocha",
        .dark = true,
        .colors = {
            .bg = LV_COLOR_MAKE(0x1E, 0x1E, 0x2E),
            .fg = LV_COLOR_MAKE(0x31, 0x32, 0x44),
            .text = LV_COLOR_MAKE(0xCD, 0xD6, 0xF4),
            .text_secondary = LV_COLOR_MAKE(0xA6, 0xAD, 0xC8),
        .accent = LV_COLOR_MAKE(0xCB, 0xA6, 0xF7),
        .selection = LV_COLOR_MAKE(0x45, 0x47, 0x5A),
        .border = LV_COLOR_MAKE(0x58, 0x5B, 0x70),
        .shadow = LV_COLOR_MAKE(0x00, 0x00, 0x00),
        .success = LV_COLOR_MAKE(0xA6, 0xE3, 0xA1),
        .warning = LV_COLOR_MAKE(0xFA, 0xB3, 0x87),
        .error = LV_COLOR_MAKE(0xF3, 0x8B, 0xA8),
        .info = LV_COLOR_MAKE(0x89, 0xB4, 0xFA),
    },
        .primary = LV_COLOR_MAKE(0xCB, 0xA6, 0xF7),
        .secondary = LV_COLOR_MAKE(0x89, 0xB4, 0xFA),
    },
    {
        .id = "solarized_light",
        .label_key = "settings.theme_solarized_light",
        .dark = false,
        .colors = {
            .bg = LV_COLOR_MAKE(0xFD, 0xF6, 0xE3),
            .fg = LV_COLOR_MAKE(0xEE, 0xE8, 0xD5),
            .text = LV_COLOR_MAKE(0x65, 0x7B, 0x83),
            .text_secondary = LV_COLOR_MAKE(0x93, 0xA1, 0xA1),
        .accent = LV_COLOR_MAKE(0x26, 0x8B, 0xD2),
        .selection = LV_COLOR_MAKE(0xEE, 0xE8, 0xD5),
        .border = LV_COLOR_MAKE(0x93, 0xA1, 0xA1),
        .shadow = LV_COLOR_MAKE(0x00, 0x00, 0x00),
        .success = LV_COLOR_MAKE(0x85, 0x99, 0x00),
        .warning = LV_COLOR_MAKE(0xB5, 0x89, 0x00),
        .error = LV_COLOR_MAKE(0xDC, 0x32, 0x2F),
        .info = LV_COLOR_MAKE(0x2A, 0xA1, 0x98),
    },
        .primary = LV_COLOR_MAKE(0x26, 0x8B, 0xD2),
        .secondary = LV_COLOR_MAKE(0x2A, 0xA1, 0x98),
    },
    {
        .id = "solarized_dark",
        .label_key = "settings.theme_solarized_dark",
        .dark = true,
        .colors = {
            .bg = LV_COLOR_MAKE(0x00, 0x2B, 0x36),
            .fg = LV_COLOR_MAKE(0x07, 0x36, 0x42),
            .text = LV_COLOR_MAKE(0x83, 0x94, 0x96),
            .text_secondary = LV_COLOR_MAKE(0x58, 0x6E, 0x75),
        .accent = LV_COLOR_MAKE(0x26, 0x8B, 0xD2),
        .selection = LV_COLOR_MAKE(0x07, 0x36, 0x42),
        .border = LV_COLOR_MAKE(0x58, 0x6E, 0x75),
        .shadow = LV_COLOR_MAKE(0x00, 0x00, 0x00),
        .success = LV_COLOR_MAKE(0x85, 0x99, 0x00),
        .warning = LV_COLOR_MAKE(0xB5, 0x89, 0x00),
        .error = LV_COLOR_MAKE(0xDC, 0x32, 0x2F),
        .info = LV_COLOR_MAKE(0x2A, 0xA1, 0x98),
    },
        .primary = LV_COLOR_MAKE(0x26, 0x8B, 0xD2),
        .secondary = LV_COLOR_MAKE(0x2A, 0xA1, 0x98),
    },
};

static size_t current_theme_index = 0;

size_t getThemeCount() {
    return sizeof(kThemes) / sizeof(kThemes[0]);
}

const ThemeDefinition& getThemeByIndex(size_t index) {
    if (index >= getThemeCount()) {
        return kThemes[0];
    }
    return kThemes[index];
}

const ThemeDefinition* findTheme(const char* id) {
    if (!id) {
        return nullptr;
    }
    for (size_t i = 0; i < getThemeCount(); ++i) {
        if (std::strcmp(kThemes[i].id, id) == 0) {
            return &kThemes[i];
        }
    }
    return nullptr;
}

const char* getThemeLabelKey(const char* id) {
    const ThemeDefinition* theme = findTheme(id);
    if (theme) {
        return theme->label_key;
    }
    const ThemeDefinition* fallback = findTheme(kDefaultThemeId);
    return fallback ? fallback->label_key : kThemes[0].label_key;
}

const char* getThemeIdByIndex(size_t index) {
    return getThemeByIndex(index).id;
}

int getThemeIndex(const char* id) {
    if (!id) {
        return -1;
    }
    for (size_t i = 0; i < getThemeCount(); ++i) {
        if (std::strcmp(kThemes[i].id, id) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

const char* getNextThemeId(const char* current_id, int delta) {
    const size_t count = getThemeCount();
    if (count == 0) {
        return kDefaultThemeId;
    }

    int index = getThemeIndex(current_id);
    if (index < 0) {
        int fallback = getThemeIndex(kDefaultThemeId);
        index = fallback >= 0 ? fallback : 0;
    }
    int next = index + delta;
    while (next < 0) {
        next += static_cast<int>(count);
    }
    next = next % static_cast<int>(count);
    return kThemes[next].id;
}

const char* getDefaultThemeId() {
    return kDefaultThemeId;
}

const char* getCurrentThemeId() {
    return getThemeByIndex(current_theme_index).id;
}

void applyTheme(const char* theme_id) {
    const ThemeDefinition* theme = findTheme(theme_id);
    if (!theme) {
        theme = findTheme(kDefaultThemeId);
        if (!theme) {
            theme = &kThemes[0];
        }
    }
    current_theme_index = static_cast<size_t>(theme - kThemes);

    lv_display_t* disp = lv_display_get_default();
    if (!disp) {
        ESP_LOGW(TAG, "No default display found for theme application");
    } else {
#if LV_USE_THEME_DEFAULT
        lv_theme_t* lv_theme = lv_theme_default_init(disp, theme->primary, theme->secondary, theme->dark,
                                                     LV_FONT_DEFAULT);
        lv_display_set_theme(disp, lv_theme);
#endif
    }

    lv_obj_t* scr = lv_screen_active();
    if (scr) {
        lv_obj_set_style_bg_color(scr, theme->colors.bg, 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(scr, theme->colors.text, 0);
        ESP_LOGI(TAG, "Applied theme: %s", theme->id);
    }
}

void applyScreenStyle(lv_obj_t* screen) {
    if (!screen) {
        return;
    }
    const Colors& colors = getColors();
    lv_obj_set_style_bg_color(screen, colors.bg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, colors.text, 0);
}

const Colors& getColors() {
    return getThemeByIndex(current_theme_index).colors;
}

bool isDark() {
    return getThemeByIndex(current_theme_index).dark;
}

// Get font based on size setting
const lv_font_t* getFont(int size_setting) {
    int size_px = size_setting;
    if (size_px <= 0) {
        size_px = FONT_SIZE_DEFAULT;
    }

    struct FontEntry {
        int size;
        const lv_font_t* font;
    };

    static const FontEntry kFontTable[] = {
#if LV_FONT_MONTSERRAT_12
        {12, &lv_font_montserrat_12},
#endif
#if LV_FONT_MONTSERRAT_14
        {14, &lv_font_montserrat_14},
#endif
#if LV_FONT_MONTSERRAT_16
        {16, &lv_font_montserrat_16},
#endif
#if LV_FONT_MONTSERRAT_18
        {18, &lv_font_montserrat_18},
#endif
#if LV_FONT_MONTSERRAT_20
        {20, &lv_font_montserrat_20},
#endif
#if LV_FONT_MONTSERRAT_22
        {22, &lv_font_montserrat_22},
#endif
#if LV_FONT_MONTSERRAT_24
        {24, &lv_font_montserrat_24},
#endif
#if LV_FONT_MONTSERRAT_26
        {26, &lv_font_montserrat_26},
#endif
#if LV_FONT_MONTSERRAT_28
        {28, &lv_font_montserrat_28},
#endif
    };

    if (sizeof(kFontTable) == 0) {
        return LV_FONT_DEFAULT;
    }

    const FontEntry* best = &kFontTable[0];
    int best_diff = std::abs(kFontTable[0].size - size_px);
    for (size_t i = 1; i < sizeof(kFontTable) / sizeof(kFontTable[0]); ++i) {
        int diff = std::abs(kFontTable[i].size - size_px);
        if (diff < best_diff) {
            best = &kFontTable[i];
            best_diff = diff;
        }
    }

    return best->font ? best->font : LV_FONT_DEFAULT;
}

} // namespace Theme
