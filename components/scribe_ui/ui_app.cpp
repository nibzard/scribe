#include "ui_app.h"
#include "display_driver.h"
#include "mipi_dsi_display.h"
#include "../scribe_export/export_sd.h"
#include "../scribe_export/send_to_computer.h"
#include "../scribe_input/keymap.h"
#include "../scribe_utils/string_utils.h"
#include "ui_screens/screen_editor.h"
#include "ui_screens/screen_menu.h"
#include "ui_screens/screen_projects.h"
#include "ui_screens/screen_export.h"
#include "ui_screens/screen_settings.h"
#include "ui_screens/screen_help.h"
#include "ui_screens/screen_recovery.h"
#include "ui_screens/screen_new_project.h"
#include "ui_screens/screen_find.h"
#include "ui_screens/screen_first_run.h"
#include "ui_screens/screen_diagnostics.h"
#include "ui_screens/screen_advanced.h"
#include "ui_screens/screen_wifi.h"
#include "ui_screens/screen_backup.h"
#include "ui_screens/screen_ai.h"
#include "ui_screens/screen_magic_bar.h"
#include "ui_screens/dialog_power_off.h"
#include "editor_core.h"
#include "project_library.h"
#include "storage_manager.h"
#include "recovery.h"
#include "session_state.h"
#include "settings_store.h"
#include "scribe_services/battery.h"
#include "scribe_services/ai_assist.h"
#include "scribe_services/github_backup.h"
#include "scribe_services/power_manager.h"
#include "scribe_services/rtc_time.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <lvgl.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <unistd.h>

static const char* TAG = "SCRIBE_UI";

// LVGL tick task
static void lvgl_tick_task(void* arg) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5));
        lv_tick_inc(5);
    }
}

UIApp& UIApp::getInstance() {
    static UIApp instance;
    return instance;
}

esp_err_t UIApp::init() {
    ESP_LOGI(TAG, "Initializing UI...");

    // Initialize LVGL
    lv_init();

    ai_event_queue_ = xQueueCreate(16, sizeof(AIEvent*));
    if (!ai_event_queue_) {
        ESP_LOGW(TAG, "Failed to create AI event queue; AI streaming will be limited");
    }

    // Try to initialize MIPI-DSI display first
    // Configure for common TFT panels via SPI (MIPI-DSI requires specific hardware)
    MIPIDSI::DisplayConfig dsi_config = {
        .panel = MIPIDSI::PanelType::ST7789,    // Common panel type
        .width = 320,                           // Tab5 display width
        .height = 240,                          // Tab5 display height
        .fps = 60,
        .use_spi = true,                        // Use SPI interface
        .spi_freq_mhz = 40,                     // 40MHz SPI
        .dc_pin = GPIO_NUM_4,                   // Data/Command pin (adjust for hardware)
        .reset_pin = GPIO_NUM_5,                // Reset pin (adjust for hardware)
        .cs_pin = GPIO_NUM_15,                  // Chip select (adjust for hardware)
        .backlight_pin = GPIO_NUM_16,           // Backlight control (adjust for hardware)
        .rgb565_byte_swap = true                // Swap bytes for RGB565
    };

    esp_err_t ret = MIPIDSI::init(dsi_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "MIPI-DSI display initialized successfully");
    } else {
        ESP_LOGW(TAG, "MIPI-DSI display init failed: %s", esp_err_to_name(ret));
        ESP_LOGI(TAG, "Falling back to generic display driver");

        // Fallback to generic display driver (for simulation/testing)
        DisplayDriver::DisplayConfig disp_config = {
            .width = 320,
            .height = 240,
            .rotation = 0,
            .double_buffer = true,
            .buffer_size = 0,
            .partial_refresh = false
        };

        ret = DisplayDriver::init(disp_config);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Generic display driver also failed: %s", esp_err_to_name(ret));
            ESP_LOGW(TAG, "UI will continue but may not display correctly");
        } else {
            ESP_LOGI(TAG, "Generic display driver initialized");
        }
    }

    // Load settings and theme
    SettingsStore& settings_store = SettingsStore::getInstance();
    settings_store.init();
    settings_store.load(settings_);
    Theme::applyTheme(settings_.dark_theme);

    // Apply keyboard layout from settings
    setKeyboardLayout(intToLayout(settings_.keyboard_layout));

    // Initialize services
    GitHubBackup& backup = GitHubBackup::getInstance();
    backup.init();
    backup.setStatusCallback([this](BackupStatus status, const std::string& message) {
        // Update HUD backup state based on status
        if (hud_visible_) {
            updateHUD();
        }
    });

    // Initialize screens
    editor_screen_ = new ScreenEditor();
    editor_screen_->init();

    menu_screen_ = new ScreenMenu();
    menu_screen_->init();

    projects_screen_ = new ScreenProjects();
    projects_screen_->init();

    new_project_screen_ = new ScreenNewProject();
    new_project_screen_->init();

    export_screen_ = new ScreenExport();
    export_screen_->init();

    settings_screen_ = new ScreenSettings();
    settings_screen_->init();

    help_screen_ = new ScreenHelp();
    help_screen_->init();

    recovery_screen_ = new ScreenRecovery();
    recovery_screen_->init();

    first_run_screen_ = new ScreenFirstRun();
    first_run_screen_->init();

    find_bar_ = new ScreenFind();
    find_bar_->init();

    diagnostics_screen_ = new ScreenDiagnostics();
    diagnostics_screen_->init();

    advanced_screen_ = new ScreenAdvanced();
    advanced_screen_->init();

    wifi_screen_ = new ScreenWiFi();
    wifi_screen_->init();

    backup_screen_ = new ScreenBackup();
    backup_screen_->init();

    // Initialize backup dialogs
    TokenInputDialog::getInstance().init();
    RepoConfigDialog::getInstance().init();

    ai_screen_ = new ScreenAI();
    ai_screen_->init();

    magic_bar_ = new ScreenMagicBar();
    magic_bar_->init();

    power_off_dialog_ = new DialogPowerOff();
    power_off_dialog_->init();

    // Set up screen callbacks
    menu_screen_->setCloseCallback([this]() { showEditor(); });

    std::vector<MenuItem> menu_items = {
        {"Resume writing", [this]() { showEditor(); }},
        {"Switch project", [this]() { showProjectSwitcher(); }},
        {"New project", [this]() { showNewProject(); }},
        {"Find", [this]() { showFind(); }},
        {"Export", [this]() { showExport(); }},
        {"Settings", [this]() { showSettings(); }},
        {"Help", [this]() { showHelp(); }},
        {"Sleep", [this]() { PowerManager::getInstance().enterSleep(); }},
        {"Power off", [this]() { showPowerOffConfirmation(); }},
    };
    menu_screen_->setItems(menu_items.data(), menu_items.size());

    projects_screen_->setCloseCallback([this]() { showMenu(); });
    projects_screen_->setOpenCallback([this](const std::string& id) {
        openProject(id);
    });

    new_project_screen_->setCreateCallback([this](const std::string& name) {
        createProject(name);
    });
    new_project_screen_->setCancelCallback([this]() {
        showMenu();
    });

    export_screen_->setCloseCallback([this]() { showEditor(); });
    export_screen_->setExportCallback([this](const std::string& type) {
        handleExport(type);
    });

    settings_screen_->setCloseCallback([this]() { showEditor(); });
    settings_screen_->setSettingChangeCallback([this](const std::string& setting, int delta) {
        if (setting == "theme") {
            settings_.dark_theme = !settings_.dark_theme;
        } else if (setting == "font_size") {
            settings_.font_size = std::max(0, std::min(2, settings_.font_size + delta));
        } else if (setting == "keyboard_layout") {
            settings_.keyboard_layout = (settings_.keyboard_layout + delta + 4) % 4;
        } else if (setting == "auto_sleep") {
            settings_.auto_sleep = std::max(0, std::min(3, settings_.auto_sleep + delta));
        }
        SettingsStore::getInstance().save(settings_);
        applySettings();
    });
    settings_screen_->setNavigateCallback([this](const char* destination) {
        if (destination && strcmp(destination, "advanced") == 0) {
            showAdvanced();
        }
    });

    help_screen_->setCloseCallback([this]() { showEditor(); });

    recovery_screen_->setRecoveryCallback([this](bool restore) {
        handleRecovery(restore);
    });

    find_bar_->setCloseCallback([this]() {
        current_screen_ = ScreenType::Editor;
    });

    first_run_screen_->setDismissCallback([this]() {
        showEditor();
    });

    advanced_screen_->setBackCallback([this]() { showSettings(); });
    advanced_screen_->setNavigateCallback([this](const char* destination) {
        if (!destination) return;
        if (strcmp(destination, "wifi") == 0) {
            showWiFi();
        } else if (strcmp(destination, "backup") == 0) {
            showBackup();
        } else if (strcmp(destination, "diagnostics") == 0) {
            showDiagnostics();
        } else if (strcmp(destination, "ai") == 0) {
            showAI();
        }
    });

    wifi_screen_->setBackCallback([this]() { showAdvanced(); });
    backup_screen_->setBackCallback([this]() { showAdvanced(); });

    // Set up backup screen configure callback
    backup_screen_->setConfigureCallback([this](BackupProvider provider) {
        GitHubBackup& backup = GitHubBackup::getInstance();
        if (!backup.isEnabled()) {
            backup.setEnabled(true);
        }
        backup.configure(provider, "", "", "main");
        // Show token input dialog if no token
        if (backup.getToken().empty()) {
            TokenInputDialog::getInstance().show(
                [this](const std::string& token) {
                    GitHubBackup::getInstance().setToken(token);
                    backup_screen_->updateStatus(true);
                },
                [this]() {
                    // Cancel - do nothing
                }
            );
        } else {
            backup_screen_->updateStatus(true);
        }
    });

    ai_screen_->setBackCallback([this]() { showAdvanced(); });
    diagnostics_screen_->setCloseCallback([this]() { showAdvanced(); });

    power_off_dialog_->setConfirmCallback([this]() {
        handlePowerOffConfirm();
    });
    power_off_dialog_->setCancelCallback([this]() {
        handlePowerOffCancel();
    });

    // Set up Magic Bar callbacks
    magic_bar_->setInsertCallback([this](const std::string& text) {
        if (editor_) {
            editor_->insert(text);
            updateEditor();
            hideMagicBar();
        }
    });
    magic_bar_->setCancelCallback([this]() {
        hideMagicBar();
    });
    magic_bar_->setStyleCallback([this](AIStyle style) {
        if (magic_bar_) {
            magic_bar_->setStyle(style);
            // Restart AI request with new style
            AIAssist& ai = AIAssist::getInstance();
            if (!ai.isEnabled()) {
                showToast("AI is off");
                return;
            }
            if (editor_) {
                ai.cancel();
                clearAIEventQueue();

                std::string context = editor_->getSelectedText();
                if (context.empty()) {
                    // Get text around cursor
                    size_t cursor = editor_->getCursor().pos;
                    size_t start = (cursor > 500) ? (cursor - 500) : 0;
                    context = editor_->getText().substr(start, 1000);
                }
                magic_bar_->setContextText(context);
                ai.requestSuggestion(style, context,
                    [this](const char* delta) {
                        AIEvent* event = new AIEvent{
                            AIEvent::Type::Delta,
                            delta ? delta : "",
                            false
                        };
                        enqueueAIEvent(event);
                    },
                    [this](bool success, const std::string& error) {
                        AIEvent* event = new AIEvent{
                            AIEvent::Type::Complete,
                            error,
                            success
                        };
                        enqueueAIEvent(event);
                    }
                );
            }
        }
    });

    applySettings();

    ESP_LOGI(TAG, "UI initialized");
    return ESP_OK;
}

esp_err_t UIApp::start() {
    if (running_) return ESP_OK;

    running_ = true;

    // Start LVGL task
    xTaskCreate(lvgl_tick_task, "lvgl_tick", 16384, nullptr, 5, nullptr);

    ESP_LOGI(TAG, "UI started");
    return ESP_OK;
}

esp_err_t UIApp::stop() {
    running_ = false;
    return ESP_OK;
}

void UIApp::setEditor(EditorCore* editor) {
    editor_ = editor;
    if (editor_screen_) {
        editor_screen_->setEditorCore(editor);
    }
}

void UIApp::showEditor() {
    ensureProjectOpen();
    if (editor_screen_) {
        editor_screen_->show();
    }
    current_screen_ = ScreenType::Editor;
    if (hud_visible_) {
        updateHUD();
    }
}

void UIApp::showMenu() {
    if (menu_screen_) {
        menu_screen_->show();
    }
    current_screen_ = ScreenType::Menu;
}

void UIApp::showProjectSwitcher() {
    if (projects_screen_) {
        // Load project list
        ProjectLibrary& lib = ProjectLibrary::getInstance();
        const auto& projects = lib.getProjects();

        std::vector<ProjectListItem> items;
        for (const auto& p : projects) {
            items.push_back({p.id, p.name, p.last_opened_utc, p.total_words});
        }

        projects_screen_->setProjects(items);
        projects_screen_->setQuery("");
        projects_screen_->show();
    }
    current_screen_ = ScreenType::Projects;
}

void UIApp::showNewProject() {
    if (new_project_screen_) {
        new_project_screen_->show();
    }
    current_screen_ = ScreenType::NewProject;
}

void UIApp::showFind() {
    if (find_bar_) {
        find_bar_->setQuery("");
        find_bar_->show();
    }
    find_matches_.clear();
    find_index_ = 0;
    current_screen_ = ScreenType::Find;
}

void UIApp::showExport() {
    if (export_screen_) {
        export_screen_->show();
    }
    export_in_progress_ = false;
    current_screen_ = ScreenType::Export;
}

void UIApp::showSettings() {
    if (settings_screen_) {
        settings_screen_->setSettings(settings_);
        settings_screen_->show();
    }
    current_screen_ = ScreenType::Settings;
}

void UIApp::showHelp() {
    if (help_screen_) {
        help_screen_->show();
    }
    current_screen_ = ScreenType::Help;
}

void UIApp::showRecovery() {
    if (recovery_screen_) {
        recovery_screen_->show(recovered_content_);
        recovery_screen_->setSelectedRestore(recovery_select_restore_);
    }
    current_screen_ = ScreenType::Recovery;
}

void UIApp::showFirstRun() {
    if (first_run_screen_) {
        first_run_screen_->show();
    }
    current_screen_ = ScreenType::FirstRun;
}

void UIApp::showDiagnostics() {
    if (diagnostics_screen_) {
        diagnostics_screen_->show();
    }
    current_screen_ = ScreenType::Diagnostics;
}

void UIApp::showAdvanced() {
    if (advanced_screen_) {
        advanced_screen_->show();
    }
    current_screen_ = ScreenType::Advanced;
}

void UIApp::showWiFi() {
    if (wifi_screen_) {
        wifi_screen_->updateStatus(settings_.wifi_enabled, false);
        wifi_screen_->show();
    }
    current_screen_ = ScreenType::WiFi;
}

void UIApp::showBackup() {
    if (backup_screen_) {
        // Initialize and update backup status
        GitHubBackup& backup = GitHubBackup::getInstance();
        backup.init();

        bool has_token = !backup.getToken().empty();
        std::string repo_info;
        if (has_token) {
            // Build repo info string
            // TODO: Get actual repo info from backup configuration
            repo_info = "Configured";
        }

        backup_screen_->updateStatus(has_token, repo_info);
        backup_screen_->show();
    }
    current_screen_ = ScreenType::Backup;
}

void UIApp::showAI() {
    if (ai_screen_) {
        ai_screen_->show();
    }
    current_screen_ = ScreenType::AI;
}

void UIApp::showMagicBar() {
    if (!magic_bar_) {
        return;
    }

    AIAssist& ai = AIAssist::getInstance();
    if (!ai.isEnabled()) {
        showToast("AI is off");
        return;
    }

    if (editor_) {
        ai.cancel();
        clearAIEventQueue();

        std::string context = editor_->getSelectedText();
        if (context.empty()) {
            // Get text around cursor
            size_t cursor = editor_->getCursor().pos;
            size_t start = (cursor > 500) ? (cursor - 500) : 0;
            context = editor_->getText().substr(start, 1000);
        }
        magic_bar_->setContextText(context);
        magic_bar_->show();

        // Start AI request
        ai.requestSuggestion(magic_bar_->getStyle(), context,
            [this](const char* delta) {
                AIEvent* event = new AIEvent{
                    AIEvent::Type::Delta,
                    delta ? delta : "",
                    false
                };
                enqueueAIEvent(event);
            },
            [this](bool success, const std::string& error) {
                AIEvent* event = new AIEvent{
                    AIEvent::Type::Complete,
                    error,
                    success
                };
                enqueueAIEvent(event);
            }
        );
    }
}

void UIApp::hideMagicBar() {
    if (magic_bar_) {
        magic_bar_->hide();
    }
    AIAssist::getInstance().cancel();
    clearAIEventQueue();
    current_screen_ = ScreenType::Editor;
}

void UIApp::processAsyncEvents() {
    if (!ai_event_queue_) {
        return;
    }

    AIEvent* event = nullptr;
    while (xQueueReceive(ai_event_queue_, &event, 0) == pdTRUE) {
        if (!event) {
            continue;
        }

        if (magic_bar_ && magic_bar_->isVisible()) {
            if (event->type == AIEvent::Type::Delta) {
                magic_bar_->onStreamDelta(event->text.c_str());
            } else if (event->type == AIEvent::Type::Complete) {
                magic_bar_->onComplete(event->success, event->text);
            }
        }

        delete event;
    }
}

void UIApp::enqueueAIEvent(AIEvent* event) {
    if (!event) {
        return;
    }

    if (!ai_event_queue_ || xQueueSend(ai_event_queue_, &event, 0) != pdTRUE) {
        delete event;
    }
}

void UIApp::clearAIEventQueue() {
    if (!ai_event_queue_) {
        return;
    }

    AIEvent* event = nullptr;
    while (xQueueReceive(ai_event_queue_, &event, 0) == pdTRUE) {
        delete event;
    }
}

void UIApp::showPowerOffConfirmation() {
    if (power_off_dialog_) {
        power_off_dialog_->show();
    }
    current_screen_ = ScreenType::PowerOff;
}

void UIApp::toggleHUD() {
    hud_visible_ = !hud_visible_;
    if (editor_screen_) {
        editor_screen_->showHUD(hud_visible_);
        if (hud_visible_) {
            updateHUD();
        }
    }
}

void UIApp::showToast(const char* message) {
    ESP_LOGI(TAG, "Toast: %s", message);
    showToastInternal(message, 2000);
}

void UIApp::updateEditor() {
    if (editor_screen_) {
        editor_screen_->update();
    }
    if (hud_visible_) {
        updateHUD();
    }
}

void UIApp::ensureProjectOpen() {
    if (!current_project_id_.empty()) {
        return;
    }

    ProjectLibrary& lib = ProjectLibrary::getInstance();
    const auto& projects = lib.getProjects();
    std::string base = "Untitled";
    std::string name = base;
    int suffix = 1;

    auto nameExists = [&](const std::string& candidate) {
        for (const auto& project : projects) {
            if (project.name == candidate) {
                return true;
            }
        }
        return false;
    };

    while (nameExists(name)) {
        ++suffix;
        name = base + " " + std::to_string(suffix);
    }

    std::string new_id;
    if (lib.createProject(name, new_id) == ESP_OK) {
        openProjectInternal(new_id);
    } else {
        ESP_LOGE(TAG, "Failed to auto-create project");
    }
}

void UIApp::createProject(const std::string& name) {
    ESP_LOGI(TAG, "Creating new project: %s", name.c_str());

    std::string trimmed = trim(name);
    if (trimmed.empty()) {
        new_project_screen_->showError("Please enter a name.");
        return;
    }

    ProjectLibrary& lib = ProjectLibrary::getInstance();
    std::string new_id;

    esp_err_t ret = lib.createProject(trimmed, new_id);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_INVALID_ARG) {
            new_project_screen_->showError("That name already exists. Try a different name.");
        } else {
            showToast("Couldn\u2019t save to storage. Your draft is still in memory.");
        }
        return;
    }

    openProjectInternal(new_id);
}

void UIApp::openProject(const std::string& id) {
    openProjectInternal(id);
}

void UIApp::openProjectInternal(const std::string& id) {
    ESP_LOGI(TAG, "Opening project: %s", id.c_str());

    ProjectLibrary& lib = ProjectLibrary::getInstance();
    const auto& projects = lib.getProjects();

    // Find project info
    for (const auto& p : projects) {
        if (p.id == id) {
            current_project_id_ = id;
            current_project_name_ = p.name;
            current_project_path_ = std::string(SCRIBE_BASE_PATH) + "/" + p.path;
            if (!current_project_path_.empty() && current_project_path_.back() != '/') {
                current_project_path_.push_back('/');
            }

            // Load manuscript content
            std::string manuscript_path = current_project_path_ + "manuscript.md";
            FILE* f = fopen(manuscript_path.c_str(), "r");
            if (f) {
                // Read file content
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                fseek(f, 0, SEEK_SET);

                std::string content;
                content.resize(size);
                fread(&content[0], 1, size, f);
                fclose(f);

                if (editor_) {
                    editor_->load(content);
                }
            } else {
                // New or empty project
                if (editor_) {
                    editor_->load("");
                }
            }

            // Update library with last opened
            lib.touchProjectOpened(id, getCurrentTimeISO());
            session_start_words_ = editor_ ? editor_->getWordCount() : 0;

            // Restore editor state
            SessionState& session = SessionState::getInstance();
            EditorState state;
            if (session.loadEditorState(id, state) == ESP_OK && editor_) {
                editor_->moveCursor(state.cursor_pos);
            }

            break;
        }
    }

    updateEditor();
    showEditor();
}

void UIApp::handleExport(const std::string& type) {
    ESP_LOGI(TAG, "Export: %s", type.c_str());

    if (!editor_) {
        export_screen_->showComplete("Export failed. Try again.");
        return;
    }

    if (type == "send_to_computer") {
        export_screen_->showSendInstructions();
        return;
    } else if (type == "sd_md") {
        // Export to SD as .md
        if (current_project_path_.empty()) {
            export_screen_->showComplete("Export failed. Try again.");
            return;
        }
        export_screen_->updateProgress();
        std::string content = editor_->getText();
        esp_err_t ret = exportToSD(current_project_path_, content, ".md");
        if (ret == ESP_OK) {
            export_screen_->showComplete("Export complete \u2713");
        } else {
            export_screen_->showComplete("Export failed. Try again.");
        }
    } else if (type == "sd_txt") {
        // Export to SD as .txt
        if (current_project_path_.empty()) {
            export_screen_->showComplete("Export failed. Try again.");
            return;
        }
        export_screen_->updateProgress();
        std::string content = editor_->getText();
        esp_err_t ret = exportToSD(current_project_path_, content, ".txt");
        if (ret == ESP_OK) {
            export_screen_->showComplete("Export complete \u2713");
        } else {
            export_screen_->showComplete("Export failed. Try again.");
        }
    }
}

void UIApp::startSendToComputer() {
    if (!editor_) {
        export_screen_->showComplete("Export failed. Try again.");
        return;
    }

    std::string content = editor_->getText();
    SendToComputer& sender = SendToComputer::getInstance();
    export_screen_->showSendRunning();

    esp_err_t ret = sender.start(content, [this](size_t sent, size_t total) {
        if (sent >= total) {
            export_in_progress_ = false;
            export_screen_->showSendDone();
        }
    });

    if (ret == ESP_OK) {
        export_in_progress_ = true;
    } else {
        export_in_progress_ = false;
        export_screen_->showSendInstructions();
        export_screen_->showComplete("Export failed. Try again.");
    }
}

void UIApp::handleRecovery(bool restore) {
    ESP_LOGI(TAG, "Recovery: %s", restore ? "restore" : "keep current");

    if (restore && editor_) {
        editor_->load(recovered_content_);
        updateEditor();
        session_start_words_ = editor_->getWordCount();
        showToast("Recovered \u2713");
    }

    if (!recovered_project_id_.empty()) {
        std::string autosave_path = getAutosaveTempPath(recovered_project_id_);
        unlink(autosave_path.c_str());
    }

    recovered_content_.clear();
    recovered_project_id_.clear();
    showEditor();
}

// Save current editor content to project
esp_err_t UIApp::saveCurrentProject() {
    if (!editor_ || current_project_id_.empty()) {
        return ESP_ERR_INVALID_STATE;
    }

    std::string content = editor_->getText();
    std::string manuscript_path = current_project_path_ + "manuscript.md";

    // Write to temp file first
    std::string tmp_path = current_project_path_ + "autosave.tmp";
    FILE* f = fopen(tmp_path.c_str(), "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create autosave.tmp");
        return ESP_FAIL;
    }

    fwrite(content.c_str(), 1, content.length(), f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);

    // Atomic rename
    if (rename(tmp_path.c_str(), manuscript_path.c_str()) != 0) {
        ESP_LOGE(TAG, "Failed to rename autosave.tmp");
        return ESP_FAIL;
    }

    // Update project library word count
    ProjectLibrary& lib = ProjectLibrary::getInstance();
    lib.updateProjectSavedState(current_project_id_, editor_->getWordCount(), getCurrentTimeISO());

    return ESP_OK;
}

bool UIApp::handleKeyEvent(const KeyEvent& event) {
    if (!event.pressed) {
        return false;
    }

    bool allow_global = true;
    if (current_screen_ == ScreenType::Export && export_screen_) {
        if (export_screen_->isSendInstructions() || export_in_progress_) {
            allow_global = false;
        }
    }

    if (allow_global) {
        if (event.ctrl && event.key == KeyEvent::Key::N) {
            showNewProject();
            return true;
        }
        if (event.ctrl && event.key == KeyEvent::Key::O) {
            showProjectSwitcher();
            return true;
        }
        if (event.ctrl && event.key == KeyEvent::Key::E) {
            showExport();
            return true;
        }
        if (event.ctrl && event.key == KeyEvent::Key::COMMA) {
            showSettings();
            return true;
        }
        if (event.ctrl && event.key == KeyEvent::Key::Q) {
            showPowerOffConfirmation();
            return true;
        }
        if (event.key == KeyEvent::Key::F1) {
            toggleHUD();
            return true;
        }
        if (event.ctrl && event.key == KeyEvent::Key::K) {
            if (magic_bar_ && magic_bar_->isVisible()) {
                hideMagicBar();
            } else {
                showMagicBar();
            }
            return true;
        }
    }

    switch (current_screen_) {
        case ScreenType::Menu:
            if (event.key == KeyEvent::Key::ESC) {
                showEditor();
                return true;
            }
            if (event.key == KeyEvent::Key::UP || (event.key == KeyEvent::Key::TAB && event.shift)) {
                menu_screen_->moveSelection(-1);
                return true;
            }
            if (event.key == KeyEvent::Key::DOWN || (event.key == KeyEvent::Key::TAB && !event.shift)) {
                menu_screen_->moveSelection(1);
                return true;
            }
            if (event.key == KeyEvent::Key::ENTER) {
                menu_screen_->selectCurrent();
                return true;
            }
            return true;
        case ScreenType::Projects:
            if (event.key == KeyEvent::Key::ESC) {
                showMenu();
                return true;
            }
            if (event.ctrl && event.key == KeyEvent::Key::N) {
                showNewProject();
                return true;
            }
            if (event.key == KeyEvent::Key::UP || (event.key == KeyEvent::Key::TAB && event.shift)) {
                projects_screen_->moveSelection(-1);
                return true;
            }
            if (event.key == KeyEvent::Key::DOWN || (event.key == KeyEvent::Key::TAB && !event.shift)) {
                projects_screen_->moveSelection(1);
                return true;
            }
            if (event.key == KeyEvent::Key::ENTER) {
                projects_screen_->selectCurrent();
                return true;
            }
            if (event.key == KeyEvent::Key::BACKSPACE) {
                projects_screen_->backspaceQuery();
                return true;
            }
            if (event.isPrintable() && !event.ctrl && !event.alt) {
                projects_screen_->appendQueryChar(event.char_code);
                return true;
            }
            return true;
        case ScreenType::NewProject:
            if (event.key == KeyEvent::Key::ESC) {
                showMenu();
                return true;
            }
            if (event.key == KeyEvent::Key::ENTER) {
                createProject(new_project_screen_->getName());
                return true;
            }
            if (event.key == KeyEvent::Key::BACKSPACE) {
                new_project_screen_->backspaceName();
                new_project_screen_->clearError();
                return true;
            }
            if (event.isPrintable() && !event.ctrl && !event.alt) {
                new_project_screen_->appendNameChar(event.char_code);
                new_project_screen_->clearError();
                return true;
            }
            return true;
        case ScreenType::Find:
            if (event.key == KeyEvent::Key::ESC) {
                find_bar_->hide();
                showEditor();
                return true;
            }
            if (event.key == KeyEvent::Key::ENTER) {
                jumpToFindMatch(event.shift ? -1 : 1);
                return true;
            }
            if (event.key == KeyEvent::Key::BACKSPACE) {
                find_bar_->backspaceQuery();
                updateFindMatches();
                return true;
            }
            if (event.isPrintable() && !event.ctrl && !event.alt) {
                find_bar_->appendQueryChar(event.char_code);
                updateFindMatches();
                return true;
            }
            return true;
        case ScreenType::Export:
            if (event.key == KeyEvent::Key::ESC) {
                if (export_screen_->isSendInstructions() || export_screen_->isSendRunning()) {
                    if (export_in_progress_) {
                        SendToComputer::getInstance().stop();
                        export_in_progress_ = false;
                    }
                    export_screen_->show();
                    return true;
                }
                showEditor();
                return true;
            }
            if (export_screen_->isSendInstructions()) {
                if (event.key == KeyEvent::Key::ENTER) {
                    startSendToComputer();
                }
                return true;
            }
            if (export_screen_->isSendRunning()) {
                if (!export_in_progress_ && event.key == KeyEvent::Key::ENTER) {
                    export_screen_->show();
                }
                return true;
            }
            if (event.key == KeyEvent::Key::UP || (event.key == KeyEvent::Key::TAB && event.shift)) {
                export_screen_->moveSelection(-1);
                return true;
            }
            if (event.key == KeyEvent::Key::DOWN || (event.key == KeyEvent::Key::TAB && !event.shift)) {
                export_screen_->moveSelection(1);
                return true;
            }
            if (event.key == KeyEvent::Key::ENTER) {
                export_screen_->selectCurrent();
                return true;
            }
            return true;
        case ScreenType::Settings:
            if (event.key == KeyEvent::Key::ESC) {
                showEditor();
                return true;
            }
            if (event.key == KeyEvent::Key::UP || (event.key == KeyEvent::Key::TAB && event.shift)) {
                settings_screen_->moveSelection(-1);
                return true;
            }
            if (event.key == KeyEvent::Key::DOWN || (event.key == KeyEvent::Key::TAB && !event.shift)) {
                settings_screen_->moveSelection(1);
                return true;
            }
            if (event.key == KeyEvent::Key::LEFT) {
                settings_screen_->adjustValue(-1);
                return true;
            }
            if (event.key == KeyEvent::Key::RIGHT) {
                settings_screen_->adjustValue(1);
                return true;
            }
            if (event.key == KeyEvent::Key::ENTER) {
                settings_screen_->selectCurrent();
                return true;
            }
            return true;
        case ScreenType::Help:
            if (event.key == KeyEvent::Key::ESC || event.key == KeyEvent::Key::ENTER) {
                showEditor();
                return true;
            }
            return true;
        case ScreenType::Recovery:
            if (event.key == KeyEvent::Key::UP || event.key == KeyEvent::Key::DOWN ||
                event.key == KeyEvent::Key::TAB) {
                recovery_select_restore_ = !recovery_select_restore_;
                recovery_screen_->setSelectedRestore(recovery_select_restore_);
                return true;
            }
            if (event.key == KeyEvent::Key::ENTER) {
                handleRecovery(recovery_select_restore_);
                return true;
            }
            if (event.key == KeyEvent::Key::ESC) {
                handleRecovery(false);
                return true;
            }
            return true;
        case ScreenType::FirstRun:
            if (event.key == KeyEvent::Key::ENTER || event.key == KeyEvent::Key::ESC) {
                showEditor();
                return true;
            }
            return true;
        case ScreenType::PowerOff:
            if (event.key == KeyEvent::Key::ESC) {
                handlePowerOffCancel();
                return true;
            }
            if (event.key == KeyEvent::Key::ENTER) {
                handlePowerOffConfirm();
                return true;
            }
            return true;
        case ScreenType::Advanced:
            if (event.key == KeyEvent::Key::ESC) {
                showSettings();
                return true;
            }
            if (event.key == KeyEvent::Key::UP || (event.key == KeyEvent::Key::TAB && event.shift)) {
                advanced_screen_->moveSelection(-1);
                return true;
            }
            if (event.key == KeyEvent::Key::DOWN || (event.key == KeyEvent::Key::TAB && !event.shift)) {
                advanced_screen_->moveSelection(1);
                return true;
            }
            if (event.key == KeyEvent::Key::ENTER) {
                advanced_screen_->selectCurrent();
                return true;
            }
            return true;
        case ScreenType::WiFi:
        case ScreenType::Backup:
        case ScreenType::AI:
        case ScreenType::Diagnostics:
            if (event.key == KeyEvent::Key::ESC) {
                showAdvanced();
                return true;
            }
            if (current_screen_ == ScreenType::WiFi && event.key == KeyEvent::Key::ENTER) {
                settings_.wifi_enabled = !settings_.wifi_enabled;
                SettingsStore::getInstance().save(settings_);
                wifi_screen_->updateStatus(settings_.wifi_enabled, false);
                return true;
            }
            return true;
        case ScreenType::Editor:
        default:
            return false;
    }
}

bool UIApp::isEditorActive() const {
    return current_screen_ == ScreenType::Editor;
}

void UIApp::setRecoveredContent(const std::string& project_id, const std::string& content) {
    recovered_project_id_ = project_id;
    recovered_content_ = content;
    recovery_select_restore_ = true;
}

void UIApp::updateHUD() {
    if (!editor_screen_ || !editor_) {
        return;
    }

    size_t total = editor_->getWordCount();
    size_t today = total >= session_start_words_ ? (total - session_start_words_) : 0;
    editor_screen_->setHUDProjectName(current_project_name_.empty() ? "Untitled" : current_project_name_);
    editor_screen_->setHUDWordCounts(today, total);

    Battery& battery = Battery::getInstance();
    battery.update();
    editor_screen_->setHUDBattery(battery.getPercentage(), battery.isCharging());

    if (saving_) {
        editor_screen_->setHUDSaveState("Saving\u2026");
    } else {
        editor_screen_->setHUDSaveState("Saved \u2713");
    }

    if (settings_.backup_enabled) {
        GitHubBackup& backup = GitHubBackup::getInstance();
        BackupStatus status = backup.getStatus();
        switch (status) {
            case BackupStatus::IDLE:
                editor_screen_->setHUDBackupState("Backup: queued");
                break;
            case BackupStatus::SYNCING:
                editor_screen_->setHUDBackupState("Backup: syncing\u2026");
                break;
            case BackupStatus::SUCCESS:
                editor_screen_->setHUDBackupState("Backup: synced \u2713");
                break;
            case BackupStatus::FAILED:
                editor_screen_->setHUDBackupState("Backup: failed");
                break;
            case BackupStatus::CONFLICT:
                editor_screen_->setHUDBackupState("Backup: conflict");
                break;
        }
    } else {
        editor_screen_->setHUDBackupState("Backup: off");
    }

    AIAssist& ai = AIAssist::getInstance();
    editor_screen_->setHUDAIState(ai.isEnabled() ? "AI: on" : "AI: off");
}

void UIApp::updateFindMatches() {
    if (!editor_ || !find_bar_) {
        return;
    }
    std::string query = find_bar_->getQuery();
    find_matches_ = editor_->find(query);
    find_index_ = 0;

    if (query.empty()) {
        find_bar_->showNoResults();
        return;
    }

    if (find_matches_.empty()) {
        find_bar_->showNoResults();
        return;
    }

    find_bar_->showMatch(1, static_cast<int>(find_matches_.size()));
    jumpToFindMatch(0);
}

void UIApp::jumpToFindMatch(int direction) {
    if (!editor_ || find_matches_.empty()) {
        find_bar_->showNoResults();
        return;
    }

    if (direction > 0) {
        find_index_ = (find_index_ + 1) % find_matches_.size();
    } else if (direction < 0) {
        if (find_index_ == 0) {
            find_index_ = find_matches_.size() - 1;
        } else {
            find_index_--;
        }
    }

    const auto& match = find_matches_[find_index_];
    editor_->moveCursor(match.start);
    updateEditor();
    find_bar_->showMatch(static_cast<int>(find_index_ + 1), static_cast<int>(find_matches_.size()));
}

void UIApp::showToastInternal(const char* message, uint32_t duration_ms) {
    if (!toast_) {
        toast_ = lv_obj_create(lv_layer_top());
        lv_obj_set_size(toast_, LV_HOR_RES - 60, 50);
        lv_obj_align(toast_, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_bg_color(toast_, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(toast_, LV_OPA_80, 0);
        lv_obj_set_style_border_width(toast_, 0, 0);
        lv_obj_add_flag(toast_, LV_OBJ_FLAG_HIDDEN);

        toast_label_ = lv_label_create(toast_);
        lv_obj_set_style_text_color(toast_label_, lv_color_white(), 0);
        lv_obj_center(toast_label_);
    }

    lv_label_set_text(toast_label_, message);
    lv_obj_clear_flag(toast_, LV_OBJ_FLAG_HIDDEN);

    if (!toast_timer_) {
        toast_timer_ = lv_timer_create([](lv_timer_t* timer) {
            UIApp* app = static_cast<UIApp*>(timer->user_data);
            if (app && app->toast_) {
                lv_obj_add_flag(app->toast_, LV_OBJ_FLAG_HIDDEN);
            }
        }, duration_ms, this);
    } else {
        lv_timer_set_period(toast_timer_, duration_ms);
        lv_timer_reset(toast_timer_);
    }
}

void UIApp::applySettings() {
    Theme::applyTheme(settings_.dark_theme);
    setKeyboardLayout(intToLayout(settings_.keyboard_layout));
    if (settings_screen_) {
        settings_screen_->setSettings(settings_);
    }
}

void UIApp::handlePowerOffConfirm() {
    if (power_off_dialog_) {
        power_off_dialog_->hide();
    }
    PowerManager::getInstance().powerOff();
}

void UIApp::handlePowerOffCancel() {
    if (power_off_dialog_) {
        power_off_dialog_->hide();
    }
    showEditor();
}
