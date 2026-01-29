#pragma once
#include <lvgl.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int size_px;
    const lv_font_t* font;
} ScribeFontEntry;

extern const lv_font_t scribe_font_montserrat_14;
extern const lv_font_t scribe_font_montserrat_16;
extern const lv_font_t scribe_font_montserrat_18;
extern const lv_font_t scribe_font_montserrat_20;
extern const lv_font_t scribe_font_montserrat_22;
extern const lv_font_t scribe_font_montserrat_24;
extern const lv_font_t scribe_font_montserrat_28;
extern const lv_font_t scribe_font_montserrat_32;
extern const lv_font_t scribe_font_montserrat_36;
extern const lv_font_t scribe_font_montserrat_40;
extern const lv_font_t scribe_font_montserrat_44;
extern const lv_font_t scribe_font_montserrat_48;
extern const lv_font_t scribe_font_montserrat_56;
extern const lv_font_t scribe_font_montserrat_64;
extern const lv_font_t scribe_font_montserrat_72;

extern const ScribeFontEntry scribe_montserrat_fonts[];
extern const size_t scribe_montserrat_font_count;

extern const lv_font_t scribe_font_dejavu_14;
extern const lv_font_t scribe_font_dejavu_16;
extern const lv_font_t scribe_font_dejavu_18;
extern const lv_font_t scribe_font_dejavu_20;
extern const lv_font_t scribe_font_dejavu_22;
extern const lv_font_t scribe_font_dejavu_24;
extern const lv_font_t scribe_font_dejavu_28;
extern const lv_font_t scribe_font_dejavu_32;
extern const lv_font_t scribe_font_dejavu_36;
extern const lv_font_t scribe_font_dejavu_40;
extern const lv_font_t scribe_font_dejavu_44;
extern const lv_font_t scribe_font_dejavu_48;
extern const lv_font_t scribe_font_dejavu_56;
extern const lv_font_t scribe_font_dejavu_64;
extern const lv_font_t scribe_font_dejavu_72;

extern const ScribeFontEntry scribe_dejavu_fonts[];
extern const size_t scribe_dejavu_font_count;

extern const lv_font_t scribe_font_ubuntu_14;
extern const lv_font_t scribe_font_ubuntu_16;
extern const lv_font_t scribe_font_ubuntu_18;
extern const lv_font_t scribe_font_ubuntu_20;
extern const lv_font_t scribe_font_ubuntu_22;
extern const lv_font_t scribe_font_ubuntu_24;
extern const lv_font_t scribe_font_ubuntu_28;
extern const lv_font_t scribe_font_ubuntu_32;
extern const lv_font_t scribe_font_ubuntu_36;
extern const lv_font_t scribe_font_ubuntu_40;
extern const lv_font_t scribe_font_ubuntu_44;
extern const lv_font_t scribe_font_ubuntu_48;
extern const lv_font_t scribe_font_ubuntu_56;
extern const lv_font_t scribe_font_ubuntu_64;
extern const lv_font_t scribe_font_ubuntu_72;

extern const ScribeFontEntry scribe_ubuntu_fonts[];
extern const size_t scribe_ubuntu_font_count;

#ifdef __cplusplus
}
#endif
