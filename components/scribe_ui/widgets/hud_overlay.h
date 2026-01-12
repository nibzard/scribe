#pragma once

#include <lvgl.h>
#include <string>

// HUD (Heads-Up Display) overlay
// Shows project info, word count, battery, save status, backup state, AI state

class HUDOverlay {
public:
    HUDOverlay(lv_obj_t* parent);
    ~HUDOverlay();

    // LVGL object access
    lv_obj_t* obj() const { return overlay_; }

    // Update HUD content
    void setProjectName(const std::string& name);
    void setWordCounts(size_t today, size_t total);
    void setBattery(int percentage, bool charging);
    void setSaveState(const std::string& state);
    void setBackupState(const std::string& state);
    void setAIState(const std::string& state);

    // Show/hide HUD
    void show();
    void hide();
    bool isVisible() const { return visible_; }

private:
    bool visible_ = false;
    lv_obj_t* overlay_ = nullptr;
    lv_obj_t* container_ = nullptr;

    // HUD labels
    lv_obj_t* project_label_ = nullptr;
    lv_obj_t* words_today_label_ = nullptr;
    lv_obj_t* words_total_label_ = nullptr;
    lv_obj_t* battery_label_ = nullptr;
    lv_obj_t* save_label_ = nullptr;
    lv_obj_t* backup_label_ = nullptr;
    lv_obj_t* ai_label_ = nullptr;

    void createWidgets();
    void updateLayout();
};
