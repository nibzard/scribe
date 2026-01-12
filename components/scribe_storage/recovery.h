#pragma once

#include <esp_err.h>
#include <string>
#include <functional>
#include <memory>

// Recovery Journal System
// Provides fine-grained crash recovery by logging edit operations

// Journal operation types
enum class JournalOpType : uint8_t {
    INSERT = 1,      // Insert characters at position
    DELETE = 2,      // Delete characters at position
    MARKER = 3       // Sync marker (checkpoint)
};

// Single journal entry
struct JournalEntry {
    JournalOpType type;
    uint32_t position;     // Character position
    uint32_t length;       // Length of insert/delete
    uint64_t timestamp;    // Microseconds since boot
    char data[1];          // Flexible array for inserted text

    // Size including data
    size_t size() const {
        return sizeof(JournalEntry) - 1 + (type == JournalOpType::INSERT ? length : 0);
    }
};

// Journal writer for recording edits
class JournalWriter {
public:
    JournalWriter(const std::string& project_id);
    ~JournalWriter();

    // Initialize journal writer
    esp_err_t init();

    // Write insert operation
    esp_err_t writeInsert(size_t position, const std::string& text);

    // Write delete operation
    esp_err_t writeDelete(size_t position, size_t length);

    // Write checkpoint marker
    esp_err_t writeMarker();

    // Sync journal to disk
    esp_err_t sync();

    // Get current journal file path
    std::string getCurrentPath() const;

    // Get number of entries written
    size_t getEntryCount() const { return entry_count_; }

private:
    std::string project_id_;
    FILE* current_file_;
    size_t entry_count_;
    size_t file_size_;        // Current journal file size
    int file_index_;          // Journal rotation index

    esp_err_t openNewFile();
    esp_err_t rotateIfNeeded();
    void closeFile();
};

// Journal reader for recovery
class JournalReader {
public:
    JournalReader(const std::string& project_id);
    ~JournalReader();

    // Initialize and find journal files
    esp_err_t init();

    // Check if any journal files exist
    bool hasJournals() const;

    // Get total number of recoverable operations
    size_t getOperationCount() const;

    // Read next operation
    // Returns nullptr when no more operations
    // Caller is responsible for freeing the returned entry
    std::unique_ptr<JournalEntry> readNext();

    // Replay all journal operations onto a text buffer
    std::string replay();

    // Clean up journal files after successful recovery
    esp_err_t cleanup();

private:
    std::string project_id_;
    FILE* current_file_;
    int file_index_;          // Current journal file being read
    size_t ops_read_;         // Operations read so far

    esp_err_t openOldestFile();
    esp_err_t openNextFile();
    void closeFile();
};

// Recovery manager - combines autosave and journal recovery
class RecoveryManager {
public:
    // Recover project state after crash
    // Returns recovered content, or empty string if nothing to recover
    static std::string recover(const std::string& project_id);

    // Clean up recovery files after successful load
    static esp_err_t cleanup(const std::string& project_id);
};

// ============================================================================
// Legacy functions (maintained for backward compatibility)
// ============================================================================

// Check if recovery files exist on boot for a project
// Returns true if autosave.tmp or journal files are found
bool checkRecoveryNeeded(const std::string& project_id);

// Read recovered content from autosave.tmp only
// For full recovery (including journal), use RecoveryManager::recover()
std::string readRecoveredContentFromAutosave(const std::string& project_id);

// Get path to recovery journal directory
std::string getRecoveryJournalDir(const std::string& project_id);

// Get path to autosave temp file
std::string getAutosaveTempPath(const std::string& project_id);
