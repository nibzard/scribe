/*
 * Scribe - Distraction-Free Writing Device
 * Main entry point for Tab5 firmware
 */

#include "app_event.h"
#include "app_queue.h"

#include "storage_manager.h"
#include "project_library.h"
#include "autosave.h"
#include "recovery.h"
#include "session_state.h"
#include "editor_core.h"
#include "ui_app.h"
#include "strings.h"
#include "keyboard_host.h"
#include "keybinding.h"
#include "space_hold_detector.h"
#include "power_manager.h"
#include "battery.h"
#include "ai_assist.h"
#include "rtc_time.h"
#include "audio_manager.h"
#include "imu_manager.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <lvgl.h>
#include <ctime>
#include <string>
#include <nvs_flash.h>

static const char* TAG = "SCRIBE_MAIN";

// Global event queue
QueueHandle_t g_event_queue = nullptr;

// Keybinding dispatcher
static KeybindingDispatcher g_keybindings;

// Forward declarations for tasks
static void ui_task(void* arg);
static void input_task(void* arg);
static void storage_task(void* arg);

// Editor instance (owned by UI task, but passed around)
static EditorCore* g_editor = nullptr;

// UI app reference
static UIApp* g_ui = nullptr;

// Storage queue for autosave requests
QueueHandle_t g_storage_queue = nullptr;
static bool g_space_tap_pending = false;

struct StorageRequest {
    EditorSnapshot snapshot;
    bool manual;
};

static SpaceHoldDetector g_space_hold;

struct RecoveryPayload {
    std::string project_id;
    std::string content;
};

static std::string getUtcTimestamp() {
    time_t now = time(nullptr);
    struct tm tm_info;
    gmtime_r(&now, &tm_info);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_info);
    return std::string(buf);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Scribe v0.1.0 starting...");
    ESP_LOGI(TAG, "Open. Type. Your words are safe.");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS and reinitializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize event queue
    g_event_queue = xQueueCreate(32, sizeof(Event));
    if (!g_event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return;
    }

    // Initialize storage queue
    g_storage_queue = xQueueCreate(4, sizeof(StorageRequest*));
    if (!g_storage_queue) {
        ESP_LOGE(TAG, "Failed to create storage queue");
        return;
    }

    // Initialize storage manager
    StorageManager& storage = StorageManager::getInstance();
    ret = storage.init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize storage: %s", esp_err_to_name(ret));
        // Continue anyway - might be able to run with in-memory storage
    }

    // Initialize autosave manager
    AutosaveManager& autosave = AutosaveManager::getInstance();
    autosave.init();

    // Initialize session state
    SessionState& session = SessionState::getInstance();
    session.init();

    // Load project library
    ProjectLibrary& library = ProjectLibrary::getInstance();
    ret = library.load();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load library, will create new");
    }

    // Initialize battery monitor
    Battery& battery = Battery::getInstance();
    battery.init();

    // Initialize power manager
    PowerManager& power = PowerManager::getInstance();
    power.init();

    // Initialize audio codec (speaker + mic)
    AudioManager& audio = AudioManager::getInstance();
    ret = audio.init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Audio init failed: %s", esp_err_to_name(ret));
    }

    // Initialize IMU (BMI270)
    ImuManager& imu = ImuManager::getInstance();
    ret = imu.init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "IMU init failed: %s", esp_err_to_name(ret));
    }

    // Initialize RTC + SNTP time sync
    initTimeSync();

    // Initialize AI assistance (optional)
    AIAssist& ai = AIAssist::getInstance();
    ai.init();

    // Initialize UI (LVGL)
    UIApp& ui = UIApp::getInstance();
    g_ui = &ui;
    ret = ui.init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize UI: %s", esp_err_to_name(ret));
        return;
    }

    // Create editor instance
    g_editor = new EditorCore();
    ui.setEditor(g_editor);

    // Set up keybinding callbacks
    g_keybindings.setMenuCallback([&ui]() {
        Event ev; ev.type = EventType::MENU_OPEN;
        xQueueSend(g_event_queue, &ev, 0);
    });
    g_keybindings.setHUDCallback([&ui]() {
        ui.toggleHUD();
    });
    g_keybindings.setNewProjectCallback([&ui]() {
        ui.showNewProject();
    });
    g_keybindings.setSwitchProjectCallback([&ui]() {
        ui.showProjectSwitcher();
    });
    g_keybindings.setExportCallback([&ui]() {
        ui.showExport();
    });
    g_keybindings.setSettingsCallback([&ui]() {
        ui.showSettings();
    });
    g_keybindings.setHelpCallback([&ui]() {
        ui.showHelp();
    });
    g_keybindings.setFindCallback([&ui]() {
        ui.showFind();
    });
    g_keybindings.setSaveCallback([]() {
        Event ev; ev.type = EventType::SAVE_REQUEST;
        xQueueSend(g_event_queue, &ev, 0);
    });

    // Editor keybindings
    g_keybindings.setInsertTextCallback([](const std::string& text) {
        if (g_editor) g_editor->insert(text);
    });
    g_keybindings.setDeleteCharCallback([](int direction) {
        if (g_editor) g_editor->deleteChar(direction);
    });
    g_keybindings.setDeleteWordCallback([](int direction) {
        if (g_editor) g_editor->deleteWord(direction);
    });
    g_keybindings.setMoveLeftCallback([](int delta) {
        if (g_editor) g_editor->moveCursorRelative(delta);
    });
    g_keybindings.setMoveRightCallback([](int delta) {
        if (g_editor) g_editor->moveCursorRelative(delta);
    });
    g_keybindings.setSelectLeftCallback([](int delta) {
        if (g_editor) g_editor->moveCursorRelativeSelect(delta);
    });
    g_keybindings.setSelectRightCallback([](int delta) {
        if (g_editor) g_editor->moveCursorRelativeSelect(delta);
    });
    g_keybindings.setMoveUpCallback([](int delta) {
        if (g_editor) g_editor->moveCursorLine(delta);
    });
    g_keybindings.setMoveDownCallback([](int delta) {
        if (g_editor) g_editor->moveCursorLine(delta);
    });
    g_keybindings.setSelectUpCallback([](int delta) {
        if (g_editor) g_editor->moveCursorLineSelect(delta);
    });
    g_keybindings.setSelectDownCallback([](int delta) {
        if (g_editor) g_editor->moveCursorLineSelect(delta);
    });
    g_keybindings.setMoveHomeCallback([]() {
        if (g_editor) g_editor->moveCursorLineStart();
    });
    g_keybindings.setMoveEndCallback([]() {
        if (g_editor) g_editor->moveCursorLineEnd();
    });
    g_keybindings.setMoveWordLeftCallback([]() {
        if (g_editor) g_editor->moveCursorWord(-1);
    });
    g_keybindings.setMoveWordRightCallback([]() {
        if (g_editor) g_editor->moveCursorWord(1);
    });
    g_keybindings.setMoveDocStartCallback([]() {
        if (g_editor) g_editor->moveCursorDocumentStart();
    });
    g_keybindings.setMoveDocEndCallback([]() {
        if (g_editor) g_editor->moveCursorDocumentEnd();
    });
    g_keybindings.setMovePageUpCallback([]() {
        if (g_editor) g_editor->moveCursorLine(-20);
    });
    g_keybindings.setMovePageDownCallback([]() {
        if (g_editor) g_editor->moveCursorLine(20);
    });
    g_keybindings.setUndoCallback([]() {
        if (g_editor) g_editor->undo();
    });
    g_keybindings.setRedoCallback([]() {
        if (g_editor) g_editor->redo();
    });
    g_keybindings.setSelectAllCallback([]() {
        if (g_editor) g_editor->selectAll();
    });
    g_keybindings.setToggleModeCallback([]() {
        if (!g_editor) return;
        EditorMode mode = g_editor->getMode();
        g_editor->setMode(mode == EditorMode::DRAFT ? EditorMode::REVISE : EditorMode::DRAFT);
    });
    g_keybindings.setAIMagicCallback([&ui]() {
        AIAssist& ai = AIAssist::getInstance();
        if (!ai.isEnabled()) {
            ui.showToast(Strings::getInstance().get("hud.ai_off"));
        } else {
            ui.showToast(Strings::getInstance().get("ai.error"));
        }
    });

    // Power management callbacks
    g_keybindings.setSleepCallback([]() {
        PowerManager& power = PowerManager::getInstance();
        power.enterSleep();
    });
    g_keybindings.setPowerOffCallback([]() {
        if (g_ui) {
            g_ui->showPowerOffConfirmation();
        }
    });

    // Initialize keyboard host
    KeyboardHost& keyboard = KeyboardHost::getInstance();
    keyboard.setCallback([](const KeyEvent& event) {
        Event ev;
        ev.type = EventType::KEY_EVENT;
        ev.key_event = event;
        xQueueSend(g_event_queue, &ev, portMAX_DELAY);
    });
    ret = keyboard.init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Keyboard init failed: %s", esp_err_to_name(ret));
    }

    // Initialize Space hold detector
    g_space_hold.init();
    g_space_hold.setHUDCallback([]() {
        Event ev;
        ev.type = EventType::HUD_TOGGLE;
        xQueueSend(g_event_queue, &ev, 0);
    });
    g_space_hold.setTapCallback([]() {
        g_space_tap_pending = true;
    });

    // Start tasks
    xTaskCreate(ui_task, "ui_task", 16384, nullptr, 5, nullptr);
    xTaskCreate(input_task, "input_task", 4096, nullptr, 4, nullptr);
    xTaskCreate(storage_task, "storage_task", 8192, nullptr, 3, nullptr);

    // Start UI
    ui.start();
    keyboard.start();

    // Load last project or show first-run tip
    const std::string& last_id = library.getLastOpenedProjectId();
    if (last_id.empty()) {
        ESP_LOGI(TAG, "No projects found. First run.");
        // Show first run tip in UI task
        Event ev;
        ev.type = EventType::SHOW_FIRST_RUN;
        xQueueSend(g_event_queue, &ev, 0);
    } else {
        ESP_LOGI(TAG, "Loading last project: %s", last_id.c_str());
        Event ev;
        ev.type = EventType::PROJECT_OPEN;
        ev.data = new std::string(last_id);
        xQueueSend(g_event_queue, &ev, 0);

        if (checkRecoveryNeeded(last_id)) {
            ESP_LOGI(TAG, "Recovery files found - will prompt user");
            RecoveryPayload* payload = new RecoveryPayload{
                last_id,
                RecoveryManager::recover(last_id)
            };
            Event rev;
            rev.type = EventType::SHOW_RECOVERY;
            rev.data = payload;
            xQueueSend(g_event_queue, &rev, 0);
        }
    }

    ESP_LOGI(TAG, "Scribe ready.");

    // Main task does nothing but wait
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));

        // Periodic status check
        if (battery.isLow()) {
            Event ev;
            ev.type = EventType::BATTERY_LOW;
            xQueueSend(g_event_queue, &ev, 0);
        }
    }
}

// UI task - owns LVGL and editor state
static void ui_task(void* arg) {
    ESP_LOGI(TAG, "UI task started");

    UIApp& ui = UIApp::getInstance();
    ui.showEditor();

    // Autosave timer tracking
    TickType_t last_keypress_time = xTaskGetTickCount();
    const TickType_t autosave_delay = pdMS_TO_TICKS(5000);  // 5 seconds idle
    const TickType_t ui_tick = pdMS_TO_TICKS(5);
    bool has_unsaved_changes = false;

    while (true) {
        Event event;
        if (xQueueReceive(g_event_queue, &event, ui_tick) == pdTRUE) {
            switch (event.type) {
                case EventType::KEY_EVENT: {
                    const KeyEvent& key_event = event.key_event;
                    bool handled = false;

                    if (ui.isEditorActive() && key_event.key == KeyEvent::Key::SPACE) {
                        g_space_tap_pending = false;
                        handled = g_space_hold.processEvent(key_event);
                        if (handled && g_space_tap_pending) {
                            g_space_tap_pending = false;
                            KeyEvent tap_event = key_event;
                            tap_event.pressed = true;
                            tap_event.shift = false;
                            tap_event.ctrl = false;
                            tap_event.alt = false;
                            tap_event.char_code = ' ';

                            if (!ui.handleKeyEvent(tap_event)) {
                                g_keybindings.processEvent(tap_event);
                                ui.updateEditor();
                            }

                            if (ui.isEditorActive()) {
                                last_keypress_time = xTaskGetTickCount();
                                has_unsaved_changes = true;
                            }
                        }
                    }

                    if (handled) {
                        break;
                    }

                    if (ui.handleKeyEvent(key_event)) {
                        break;
                    }

                    g_keybindings.processEvent(key_event);
                    ui.updateEditor();

                    if (ui.isEditorActive() &&
                        (key_event.isPrintable() ||
                         key_event.key == KeyEvent::Key::ENTER ||
                         key_event.key == KeyEvent::Key::TAB ||
                         key_event.key == KeyEvent::Key::BACKSPACE ||
                         key_event.key == KeyEvent::Key::DELETE ||
                         (key_event.ctrl && (key_event.key == KeyEvent::Key::Z ||
                                             key_event.key == KeyEvent::Key::Y)))) {
                        last_keypress_time = xTaskGetTickCount();
                        has_unsaved_changes = true;
                    }
                    break;
                }
                case EventType::MENU_OPEN:
                    ui.showMenu();
                    break;
                case EventType::MENU_CLOSE:
                    ui.showEditor();
                    break;
                case EventType::HUD_TOGGLE:
                    ui.toggleHUD();
                    break;
                case EventType::SAVE_REQUEST: {
                    // Manual save - trigger snapshot
                    if (g_editor) {
                        std::string project_id = ui.getCurrentProjectId();
                        if (project_id.empty()) {
                            ui.showToast(Strings::getInstance().get("storage.write_error"));
                            break;
                        }
                        EditorSnapshot snap = g_editor->createSnapshot(project_id);
                        StorageRequest* req = new StorageRequest{
                            snap,
                            true
                        };
                        if (xQueueSend(g_storage_queue, &req, 0) != pdTRUE) {
                            delete req;
                            ui.showToast(Strings::getInstance().get("storage.write_error"));
                        } else {
                            ui.setSaving(true);
                        }
                    }
                    break;
                }
                case EventType::STORAGE_SAVE_DONE:
                    ui.setSaving(false);
                    if (event.int_param == 1) {
                        ui.showToast(Strings::getInstance().get("hud.saved"));
                    }
                    break;
                case EventType::STORAGE_ERROR:
                    ui.setSaving(false);
                    ui.showToast(Strings::getInstance().get("storage.write_error"));
                    break;
                case EventType::BATTERY_LOW:
                    ui.showToast(Strings::getInstance().get("power.low_battery"));
                    break;
                case EventType::SHOW_FIRST_RUN:
                    ui.showFirstRun();
                    break;
                case EventType::PROJECT_OPEN: {
                    std::string* project_id = static_cast<std::string*>(event.data);
                    if (project_id) {
                        ui.openProject(*project_id);
                        delete project_id;
                    }
                    break;
                }
                case EventType::SHOW_RECOVERY:
                    if (event.data) {
                        RecoveryPayload* payload = static_cast<RecoveryPayload*>(event.data);
                        ui.setRecoveredContent(payload->project_id, payload->content);
                        delete payload;
                    }
                    ui.showRecovery();
                    break;
                default:
                    break;
            }
        }

        // Check for autosave trigger (every 5 seconds of idle time)
        TickType_t current_time = xTaskGetTickCount();
        if (has_unsaved_changes && (current_time - last_keypress_time >= autosave_delay)) {
            // Trigger autosave
            if (g_editor) {
                std::string project_id = ui.getCurrentProjectId();
                if (!project_id.empty()) {
                    EditorSnapshot snap = g_editor->createSnapshot(project_id);
                    StorageRequest* req = new StorageRequest{
                        snap,
                        false
                    };
                    if (xQueueSend(g_storage_queue, &req, 0) != pdTRUE) {
                        delete req;
                    } else {
                        ui.setSaving(true);
                    }
                }
                has_unsaved_changes = false;
                ESP_LOGI(TAG, "Autosave triggered");
            }
        }

        ui.processAsyncEvents();
        lv_timer_handler();
    }

    vTaskDelete(nullptr);
}

// Input task - reads USB keyboard and sends events
static void input_task(void* arg) {
    ESP_LOGI(TAG, "Input task started");
    (void)arg;

    while (true) {
        // Keyboard callback sends events to queue
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(nullptr);
}

// Storage task - saves documents in background
static void storage_task(void* arg) {
    ESP_LOGI(TAG, "Storage task started");
    AutosaveManager& autosave = AutosaveManager::getInstance();
    ProjectLibrary& library = ProjectLibrary::getInstance();
    SessionState& session = SessionState::getInstance();
    StorageManager& storage = StorageManager::getInstance();

    while (true) {
        StorageRequest* req = nullptr;
        if (xQueueReceive(g_storage_queue, &req, portMAX_DELAY) == pdTRUE) {
            if (!req) {
                continue;
            }
            if (!storage.isMounted()) {
                Event ev;
                ev.type = EventType::STORAGE_ERROR;
                xQueueSend(g_event_queue, &ev, 0);
                delete req;
                continue;
            }

            ESP_LOGI(TAG, "Saving project %s (%zu words)",
                     req->snapshot.project_id.c_str(), req->snapshot.word_count);

            DocSnapshot snap{
                .project_id = req->snapshot.project_id,
                .table = req->snapshot.table,
                .word_count = req->snapshot.word_count,
                .timestamp = static_cast<uint64_t>(time(nullptr))
            };

            esp_err_t ret = req->manual
                ? autosave.saveNow(snap)
                : autosave.queueSave(snap, nullptr);

            if (ret == ESP_OK) {
                std::string saved_time = getUtcTimestamp();
                library.updateProjectSavedState(req->snapshot.project_id, req->snapshot.word_count, saved_time);

                EditorState state;
                state.cursor_pos = req->snapshot.cursor_pos;
                state.scroll_offset = 0;
                state.last_edit_time = static_cast<uint64_t>(time(nullptr));
                session.saveEditorState(req->snapshot.project_id, state);

                Event ev;
                ev.type = EventType::STORAGE_SAVE_DONE;
                ev.int_param = req->manual ? 1 : 0;
                xQueueSend(g_event_queue, &ev, 0);
            } else {
                Event ev;
                ev.type = EventType::STORAGE_ERROR;
                ev.int_param = req->manual ? 1 : 0;
                xQueueSend(g_event_queue, &ev, 0);
            }

            delete req;
        }
    }

    vTaskDelete(nullptr);
}
