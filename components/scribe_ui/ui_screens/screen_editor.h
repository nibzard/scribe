#pragma once

#include <lvgl.h>
#include "../widgets/text_view.h"
#include "editor_core.h"
#include <string>

// Main editor screen - full-screen text editor
class ScreenEditor {
public:
    ScreenEditor();
    ~ScreenEditor();

    // Initialize the editor screen
    void init();

    // Show/hide screen
    void show();
    void hide();

    // Load document content
    void loadContent(const std::string& content);

    // Get current content
    std::string getContent() const;

    // Set editor core reference
    void setEditorCore(EditorCore* editor) { editor_ = editor; }

    // Update display (called after edits)
    void update();

    // Show/hide HUD
    void showHUD(bool show);
    void setHUDProjectName(const std::string& name);
    void setHUDWordCounts(size_t today, size_t total);
    void setHUDBattery(int percentage, bool charging);
    void setHUDSaveState(const std::string& status);
    void setHUDBackupState(const std::string& status);
    void setHUDAIState(const std::string& status);

private:
    lv_obj_t* screen_;
    TextView* text_view_ = nullptr;
    lv_obj_t* hud_panel_;
    lv_obj_t* hud_project_label_ = nullptr;
    lv_obj_t* hud_project_value_ = nullptr;
    lv_obj_t* hud_today_label_ = nullptr;
    lv_obj_t* hud_today_value_ = nullptr;
    lv_obj_t* hud_total_label_ = nullptr;
    lv_obj_t* hud_total_value_ = nullptr;
    lv_obj_t* hud_battery_label_ = nullptr;
    lv_obj_t* hud_battery_value_ = nullptr;
    lv_obj_t* hud_save_label_ = nullptr;
    lv_obj_t* hud_backup_label_ = nullptr;
    lv_obj_t* hud_ai_label_ = nullptr;

    EditorCore* editor_ = nullptr;
    bool hud_visible_ = false;
    uint64_t last_revision_ = 0;

    void createWidgets();
    void updateHUD();
};
