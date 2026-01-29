#include "theme.h"
#include "generated/scribe_fonts.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include <lvgl.h>

extern "C" {
extern const unsigned char _binary_montserrat_ttf_start[];
extern const unsigned char _binary_montserrat_ttf_end[];
extern const unsigned char _binary_dejavu_ttf_start[];
extern const unsigned char _binary_dejavu_ttf_end[];
extern const unsigned char _binary_ubuntu_ttf_start[];
extern const unsigned char _binary_ubuntu_ttf_end[];
}

namespace {
struct FontFamilyDefinition {
    const char* id;
    const char* label_key;
    const unsigned char* data;
    size_t size;
};

constexpr int kUiFontCacheSize = 256;
// Avoid TinyTTF cache eviction crashes on ESP32-P4 by disabling editor glyph caching.
constexpr int kEditorFontCacheSize = 0;
// Temporary safety switch: disable TinyTTF usage until cache corruption is resolved.
constexpr bool kEnableTinyTtf = false;
constexpr int kUiFontBaseSizes[] = {14, 16, 20, 24};

const FontFamilyDefinition kFontFamilies[] = {
    {
        "montserrat",
        "settings.font_montserrat",
        _binary_montserrat_ttf_start,
        static_cast<size_t>(_binary_montserrat_ttf_end - _binary_montserrat_ttf_start)
    },
    {
        "dejavu",
        "settings.font_dejavu",
        _binary_dejavu_ttf_start,
        static_cast<size_t>(_binary_dejavu_ttf_end - _binary_dejavu_ttf_start)
    },
    {
        "ubuntu",
        "settings.font_ubuntu",
        _binary_ubuntu_ttf_start,
        static_cast<size_t>(_binary_ubuntu_ttf_end - _binary_ubuntu_ttf_start)
    }
};

constexpr size_t kUiFontRoleCount = sizeof(kUiFontBaseSizes) / sizeof(kUiFontBaseSizes[0]);
static_assert(kUiFontRoleCount == 4, "UiFontRole count mismatch");

struct EditorFontEntry {
    const FontFamilyDefinition* family = nullptr;
    lv_font_t* font = nullptr;
    int size_px = 0;
};

#if LV_USE_TINY_TTF
static lv_font_t* ui_fonts[kUiFontRoleCount] = {nullptr, nullptr, nullptr, nullptr};
static int ui_font_sizes[kUiFontRoleCount] = {0, 0, 0, 0};
static EditorFontEntry editor_fonts[sizeof(kFontFamilies) / sizeof(kFontFamilies[0])];
#endif

static int current_ui_scale = Theme::UI_SCALE_DEFAULT;
static const char* kDefaultEditorFontId = "montserrat";
struct EditorFontFamilyMap {
    const char* id = nullptr;
    const ScribeFontEntry* fonts = nullptr;
    size_t count = 0;
};

static const EditorFontFamilyMap kEditorFontFamilies[] = {
    { "montserrat", scribe_montserrat_fonts, scribe_montserrat_font_count },
    { "dejavu", scribe_dejavu_fonts, scribe_dejavu_font_count },
    { "ubuntu", scribe_ubuntu_fonts, scribe_ubuntu_font_count },
};

const EditorFontFamilyMap* findEditorFamily(const char* id) {
    if (!id) {
        return nullptr;
    }
    for (const auto& family : kEditorFontFamilies) {
        if (std::strcmp(family.id, id) == 0) {
            return &family;
        }
    }
    return nullptr;
}

const lv_font_t* getNearestPrecompiledFont(const EditorFontFamilyMap& family, int size_px) {
    if (!family.fonts || family.count == 0) {
        return nullptr;
    }
    const ScribeFontEntry* best = &family.fonts[0];
    int best_diff = std::abs(best->size_px - size_px);
    for (size_t i = 1; i < family.count; ++i) {
        const ScribeFontEntry* entry = &family.fonts[i];
        int diff = std::abs(entry->size_px - size_px);
        if (diff < best_diff) {
            best = entry;
            best_diff = diff;
        }
    }
    return best->font;
}

const FontFamilyDefinition* findFontFamily(const char* id) {
    if (!id) {
        return nullptr;
    }
    for (const auto& family : kFontFamilies) {
        if (std::strcmp(family.id, id) == 0) {
            return &family;
        }
    }
    return nullptr;
}

size_t getFontFamilyIndex(const char* id) {
    for (size_t i = 0; i < sizeof(kFontFamilies) / sizeof(kFontFamilies[0]); ++i) {
        if (std::strcmp(kFontFamilies[i].id, id) == 0) {
            return i;
        }
    }
    return 0;
}

const lv_font_t* getBuiltinFont(int size_px) {
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
#if LV_FONT_MONTSERRAT_30
        {30, &lv_font_montserrat_30},
#endif
#if LV_FONT_MONTSERRAT_32
        {32, &lv_font_montserrat_32},
#endif
#if LV_FONT_MONTSERRAT_34
        {34, &lv_font_montserrat_34},
#endif
#if LV_FONT_MONTSERRAT_36
        {36, &lv_font_montserrat_36},
#endif
#if LV_FONT_MONTSERRAT_38
        {38, &lv_font_montserrat_38},
#endif
#if LV_FONT_MONTSERRAT_40
        {40, &lv_font_montserrat_40},
#endif
#if LV_FONT_MONTSERRAT_42
        {42, &lv_font_montserrat_42},
#endif
#if LV_FONT_MONTSERRAT_44
        {44, &lv_font_montserrat_44},
#endif
#if LV_FONT_MONTSERRAT_46
        {46, &lv_font_montserrat_46},
#endif
#if LV_FONT_MONTSERRAT_48
        {48, &lv_font_montserrat_48},
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

#if LV_USE_TINY_TTF
lv_font_t* createTinyTtfFont(const FontFamilyDefinition& family, int size_px, size_t cache_size) {
    if (!family.data || family.size == 0) {
        return nullptr;
    }
    lv_font_t* font = lv_tiny_ttf_create_data_ex(family.data, family.size, size_px,
                                                 LV_FONT_KERNING_NORMAL, cache_size);
    return font;
}
#endif

void refreshUiFonts() {
#if LV_USE_TINY_TTF
    if (!kEnableTinyTtf) {
        return;
    }
    const FontFamilyDefinition* family = findFontFamily(kDefaultEditorFontId);
    if (!family) {
        family = &kFontFamilies[0];
    }
    for (size_t i = 0; i < kUiFontRoleCount; ++i) {
        int target_size = Theme::scalePx(kUiFontBaseSizes[i]);
        if (target_size < 1) {
            target_size = 1;
        }
        if (!ui_fonts[i]) {
            ui_fonts[i] = createTinyTtfFont(*family, target_size, kUiFontCacheSize);
            ui_font_sizes[i] = target_size;
        } else if (ui_font_sizes[i] != target_size) {
            lv_tiny_ttf_set_size(ui_fonts[i], target_size);
            ui_font_sizes[i] = target_size;
        }
    }
#else
#endif
}

} // namespace

namespace Theme {

void setUiScalePercent(int percent) {
    if (percent < UI_SCALE_MIN) {
        percent = UI_SCALE_MIN;
    } else if (percent > UI_SCALE_MAX) {
        percent = UI_SCALE_MAX;
    }
    if (current_ui_scale == percent) {
        return;
    }
    current_ui_scale = percent;
    refreshUiFonts();
}

int getUiScalePercent() {
    return current_ui_scale;
}

int scalePx(int base_px) {
    if (base_px == 0) {
        return 0;
    }
    int scaled = (base_px * current_ui_scale + (UI_SCALE_BASE / 2)) / UI_SCALE_BASE;
    return scaled > 0 ? scaled : 1;
}

int fitWidth(int base_px, int margin_px) {
    int scaled = scalePx(base_px);
    int max_width = LV_HOR_RES - scalePx(margin_px);
    if (max_width < 1) {
        max_width = 1;
    }
    return scaled > max_width ? max_width : scaled;
}

int fitHeight(int base_px, int margin_px) {
    int scaled = scalePx(base_px);
    int max_height = LV_VER_RES - scalePx(margin_px);
    if (max_height < 1) {
        max_height = 1;
    }
    return scaled > max_height ? max_height : scaled;
}

const char* getDefaultEditorFontId() {
    return kDefaultEditorFontId;
}

const char* getEditorFontLabelKey(const char* id) {
    const FontFamilyDefinition* family = findFontFamily(id);
    if (family) {
        return family->label_key;
    }
    const FontFamilyDefinition* fallback = findFontFamily(kDefaultEditorFontId);
    return fallback ? fallback->label_key : kFontFamilies[0].label_key;
}

const char* getNextEditorFontId(const char* current_id, int delta) {
    size_t count = sizeof(kFontFamilies) / sizeof(kFontFamilies[0]);
    if (count == 0) {
        return kDefaultEditorFontId;
    }
    size_t index = getFontFamilyIndex(current_id ? current_id : kDefaultEditorFontId);
    int next = static_cast<int>(index) + delta;
    while (next < 0) {
        next += static_cast<int>(count);
    }
    next = next % static_cast<int>(count);
    return kFontFamilies[next].id;
}

size_t getEditorFontCount() {
    return sizeof(kFontFamilies) / sizeof(kFontFamilies[0]);
}

const char* getEditorFontIdByIndex(size_t index) {
    if (index >= getEditorFontCount()) {
        return kDefaultEditorFontId;
    }
    return kFontFamilies[index].id;
}

int getEditorFontIndex(const char* id) {
    return static_cast<int>(getFontFamilyIndex(id ? id : kDefaultEditorFontId));
}

int clampEditorFontSize(int size_px) {
    if (size_px < FONT_SIZE_MIN) {
        return FONT_SIZE_MIN;
    }
    if (size_px > FONT_SIZE_MAX) {
        return FONT_SIZE_MAX;
    }
    return size_px;
}

const lv_font_t* getEditorFont(const char* font_id, int size_px) {
    size_px = clampEditorFontSize(size_px);
#if LV_USE_TINY_TTF
    if (kEnableTinyTtf) {
        const FontFamilyDefinition* family = findFontFamily(font_id);
        if (!family) {
            family = findFontFamily(kDefaultEditorFontId);
            if (!family) {
                family = &kFontFamilies[0];
            }
        }
        size_t index = getFontFamilyIndex(family->id);
        EditorFontEntry& entry = editor_fonts[index];
        if (!entry.font) {
            entry.font = createTinyTtfFont(*family, size_px, kEditorFontCacheSize);
            entry.family = family;
            entry.size_px = size_px;
        } else if (entry.size_px != size_px) {
            lv_tiny_ttf_set_size(entry.font, size_px);
            entry.size_px = size_px;
        }
        if (entry.font) {
            return entry.font;
        }
    }
#endif

    const EditorFontFamilyMap* family = findEditorFamily(font_id);
    if (!family) {
        family = findEditorFamily(kDefaultEditorFontId);
    }
    if (family) {
        const lv_font_t* font = getNearestPrecompiledFont(*family, size_px);
        if (font) {
            return font;
        }
    }

    (void)font_id;
    return getBuiltinFont(size_px);
}

const lv_font_t* getUIFont(UiFontRole role) {
#if LV_USE_TINY_TTF
    if (!kEnableTinyTtf) {
        int fallback_size = kUiFontBaseSizes[static_cast<size_t>(role)];
        return getBuiltinFont(scalePx(fallback_size));
    }
    refreshUiFonts();
    size_t index = static_cast<size_t>(role);
    if (index < kUiFontRoleCount && ui_fonts[index]) {
        return ui_fonts[index];
    }
#endif
    int fallback_size = kUiFontBaseSizes[static_cast<size_t>(role)];
    return getBuiltinFont(scalePx(fallback_size));
}

} // namespace Theme
