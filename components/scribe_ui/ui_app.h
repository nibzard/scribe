#pragma once

#include <esp_err.h>
#include <string>
#include <vector>
#include "scribe_input/key_event.h"
#include "scribe_storage/settings_store.h"
#include "scribe_editor/editor_core.h"

// Forward declarations
class EditorCore;
class ScreenEditor;
class ScreenMenu;
class ScreenProjects;
class ScreenExport;
class ScreenSettings;
class ScreenHelp;
class ScreenRecovery;
class ScreenNewProject;
class ScreenFind;
class ScreenFirstRun;
class ScreenDiagnostics;
class ScreenAdvanced;
class ScreenWiFi;
class ScreenBackup;
class DialogPowerOff;

// Main UI application class
// Manages LVGL initialization, screen routing, and event handling

class UIApp {
public:
    static UIApp& getInstance();

    // Initialize LVGL and display
    esp_err_t init();

    // Start UI task
    esp_err_t start();

    // Stop UI task
    esp_err_t stop();

    // Set the editor core instance
    void setEditor(EditorCore* editor);

    // Show editor screen (default)
    void showEditor();

    // Show menu screen
    void showMenu();

    // Show project switcher
    void showProjectSwitcher();

    // Show new project dialog
    void showNewProject();

    // Open a project by ID
    void openProject(const std::string& id);

    // Show find bar
    void showFind();

    // Show export screen
    void showExport();

    // Show settings
    void showSettings();

    // Show help
    void showHelp();

    // Show recovery dialog
    void showRecovery();

    // Show first run tip
    void showFirstRun();

    // Show diagnostics
    void showDiagnostics();

    // Show advanced settings
    void showAdvanced();

    // Show WiFi settings
    void showWiFi();

    // Show backup settings
    void showBackup();

    // Show power off confirmation
    void showPowerOffConfirmation();

    // Toggle HUD (F1 or Space hold)
    void toggleHUD();

    // Show toast notification
    void showToast(const char* message);

    // Update editor display
    void updateEditor();

    // Save current project
    esp_err_t saveCurrentProject();

    // Input handling
    bool handleKeyEvent(const KeyEvent& event);
    bool isEditorActive() const;

    const std::string& getCurrentProjectId() const { return current_project_id_; }
    const std::string& getCurrentProjectPath() const { return current_project_path_; }
    const std::string& getCurrentProjectName() const { return current_project_name_; }
    void setRecoveredContent(const std::string& project_id, const std::string& content);
    void setSaving(bool saving) { saving_ = saving; }
    bool isSaving() const { return saving_; }

private:
    UIApp() : running_(false), hud_visible_(false),
              editor_screen_(nullptr), menu_screen_(nullptr),
              projects_screen_(nullptr), new_project_screen_(nullptr),
              export_screen_(nullptr), settings_screen_(nullptr),
              help_screen_(nullptr), recovery_screen_(nullptr),
              first_run_screen_(nullptr), find_bar_(nullptr),
              diagnostics_screen_(nullptr), advanced_screen_(nullptr),
              wifi_screen_(nullptr), backup_screen_(nullptr), power_off_dialog_(nullptr) {}
    ~UIApp() = default;

    bool running_;
    bool hud_visible_;
    bool export_in_progress_ = false;
    bool recovery_select_restore_ = true;
    bool saving_ = false;
    size_t session_start_words_ = 0;
    size_t find_index_ = 0;
    std::string recovered_content_;
    std::string recovered_project_id_;
    std::vector<EditorCore::Match> find_matches_;

    std::string current_project_id_;
    std::string current_project_path_;
    std::string current_project_name_;
    AppSettings settings_;

    enum class ScreenType {
        Editor,
        Menu,
        Projects,
        NewProject,
        Export,
        Settings,
        Help,
        Recovery,
        FirstRun,
        Find,
        Diagnostics,
        Advanced,
        WiFi,
        Backup,
        PowerOff
    };
    ScreenType current_screen_ = ScreenType::Editor;
    EditorCore* editor_ = nullptr;

    // Screen instances
    ScreenEditor* editor_screen_;
    ScreenMenu* menu_screen_;
    ScreenProjects* projects_screen_;
    ScreenNewProject* new_project_screen_;
    ScreenExport* export_screen_;
    ScreenSettings* settings_screen_;
    ScreenHelp* help_screen_;
    ScreenRecovery* recovery_screen_;
    ScreenFirstRun* first_run_screen_;
    ScreenFind* find_bar_;
    ScreenDiagnostics* diagnostics_screen_;
    ScreenAdvanced* advanced_screen_;
    ScreenWiFi* wifi_screen_;
    ScreenBackup* backup_screen_;
    DialogPowerOff* power_off_dialog_;
    lv_obj_t* toast_ = nullptr;
    lv_obj_t* toast_label_ = nullptr;
    lv_timer_t* toast_timer_ = nullptr;

    // Helper methods
    void createProject(const std::string& name);
    void ensureProjectOpen();
    void openProjectInternal(const std::string& id);
    void handleExport(const std::string& type);
    void startSendToComputer();
    void handleRecovery(bool restore);
    void handlePowerOffConfirm();
    void handlePowerOffCancel();
    void updateHUD();
    void updateFindMatches();
    void jumpToFindMatch(int direction);
    void showToastInternal(const char* message, uint32_t duration_ms);
    void applySettings();
};
