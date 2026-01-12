#pragma once

#include <lvgl.h>
#include <cstdint>
#include <cstring>

// HUD (Heads-Up Display) overlay
// Shows project info, word count, battery, save status

class HUDOverlay : public lv_obj {
public:
    HUDOverlay(lv_obj_t* parent);

    // Update HUD content
    void setProjectName(const char* name);
    void setWordCount(size_t today, size_t total);
    void setBattery(int percentage, bool charging);
    void setSaveState(bool saved, bool backing_up = false);

    // Show/hide HUD
    void show();
    void hide();
    bool isVisible() const { return visible_; }

private:
    bool visible_ = false;

    lv_obj_t* container_ = nullptr;
    lv_obj_t* project_label_ = nullptr;
    lv_obj_t* words_today_label_ = nullptr;
    lv_obj_t* words_total_label_ = nullptr;
    lv_obj_t* battery_label_ = nullptr;
    lv_obj_t* save_label_ = nullptr;

    void createWidgets();
};
