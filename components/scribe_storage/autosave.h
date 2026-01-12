#pragma once

#include <esp_err.h>
#include <string>
#include <functional>

// Document snapshot for safe background saves
struct DocSnapshot {
    std::string project_id;
    std::string content;
    size_t word_count;
    uint64_t timestamp;
};

// Callback type for save completion
using SaveCallback = std::function<void(esp_err_t)>;

// Autosave manager - runs on storage task
class AutosaveManager {
public:
    static AutosaveManager& getInstance();

    // Initialize autosave timer
    esp_err_t init();

    // Queue a save request (non-blocking)
    esp_err_t queueSave(const DocSnapshot& snapshot, SaveCallback callback);

    // Manual save (Ctrl+S) - forces immediate save + snapshot
    esp_err_t saveNow(const DocSnapshot& snapshot);

    // Get last save status for HUD
    bool isSaving() const { return saving_; }

private:
    AutosaveManager() = default;
    ~AutosaveManager() = default;

    bool saving_ = false;

    // Perform actual save operation
    esp_err_t performSave(const DocSnapshot& snapshot);

    // Create snapshot file (manuscript.md.~N)
    esp_err_t createSnapshot(const std::string& project_path);

    // Atomic save: tmp -> rename
    esp_err_t atomicSave(const std::string& project_path, const std::string& content);
};
