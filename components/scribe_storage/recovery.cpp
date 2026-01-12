#include "recovery.h"
#include "storage_manager.h"
#include "autosave.h"
#include <esp_log.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <algorithm>

static const char* TAG = "SCRIBE_RECOVERY";

// Journal configuration
constexpr size_t MAX_JOURNAL_FILE_SIZE = 64 * 1024;  // 64KB per journal file
constexpr size_t JOURNAL_SYNC_INTERVAL = 5;           // Sync every 5 operations

// ============================================================================
// Path helpers
// ============================================================================

std::string getAutosaveTempPath(const std::string& project_id) {
    return std::string(SCRIBE_PROJECTS_DIR) + "/" + project_id + "/autosave.tmp";
}

std::string getRecoveryJournalDir(const std::string& project_id) {
    return std::string(SCRIBE_PROJECTS_DIR) + "/" + project_id + "/journal";
}

std::string getJournalFilePath(const std::string& project_id, int index) {
    return getRecoveryJournalDir(project_id) + "/edit_" + std::to_string(index) + ".jnl";
}

// ============================================================================
// JournalWriter Implementation
// ============================================================================

JournalWriter::JournalWriter(const std::string& project_id)
    : project_id_(project_id)
    , current_file_(nullptr)
    , entry_count_(0)
    , file_size_(0)
    , file_index_(0)
{
}

JournalWriter::~JournalWriter() {
    closeFile();
}

esp_err_t JournalWriter::init() {
    // Create journal directory
    std::string journal_dir = getRecoveryJournalDir(project_id_);
    struct stat st;
    if (stat(journal_dir.c_str(), &st) != 0) {
        mkdir(journal_dir.c_str(), 0755);
    }

    // Find the next available journal file index
    DIR* dir = opendir(journal_dir.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strncmp(entry->d_name, "edit_", 5) == 0) {
                int idx = atoi(entry->d_name + 5);
                if (idx >= file_index_) {
                    file_index_ = idx + 1;
                }
            }
        }
        closedir(dir);
    }

    return openNewFile();
}

esp_err_t JournalWriter::openNewFile() {
    closeFile();

    std::string path = getJournalFilePath(project_id_, file_index_);
    current_file_ = fopen(path.c_str(), "wb");
    if (!current_file_) {
        ESP_LOGE(TAG, "Failed to create journal file: %s", path.c_str());
        return ESP_FAIL;
    }

    file_size_ = 0;
    entry_count_ = 0;
    ESP_LOGI(TAG, "Opened new journal file: %s", path.c_str());
    return ESP_OK;
}

void JournalWriter::closeFile() {
    if (current_file_) {
        fflush(current_file_);
        fclose(current_file_);
        current_file_ = nullptr;
    }
}

esp_err_t JournalWriter::rotateIfNeeded() {
    if (file_size_ >= MAX_JOURNAL_FILE_SIZE) {
        file_index_++;
        return openNewFile();
    }
    return ESP_OK;
}

esp_err_t JournalWriter::writeInsert(size_t position, const std::string& text) {
    if (!current_file_) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t text_len = text.length();
    size_t entry_size = sizeof(JournalOpType) + sizeof(uint32_t) * 2 + sizeof(uint64_t) + sizeof(uint32_t) + text_len;

    // Allocate entry buffer
    uint8_t* buffer = (uint8_t*)malloc(entry_size);
    if (!buffer) {
        return ESP_ERR_NO_MEM;
    }

    uint8_t* ptr = buffer;

    // Write type
    JournalOpType type = JournalOpType::INSERT;
    memcpy(ptr, &type, sizeof(JournalOpType));
    ptr += sizeof(JournalOpType);

    // Write position
    uint32_t pos = static_cast<uint32_t>(position);
    memcpy(ptr, &pos, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    // Write length
    uint32_t len = static_cast<uint32_t>(text_len);
    memcpy(ptr, &len, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    // Write timestamp
    uint64_t timestamp = esp_timer_get_time();
    memcpy(ptr, &timestamp, sizeof(uint64_t));
    ptr += sizeof(uint64_t);

    // Write text data
    memcpy(ptr, text.c_str(), text_len);
    ptr += text_len;

    // Write to file
    size_t written = fwrite(buffer, 1, entry_size, current_file_);
    free(buffer);

    if (written != entry_size) {
        ESP_LOGE(TAG, "Failed to write journal entry");
        return ESP_FAIL;
    }

    file_size_ += entry_size;
    entry_count_++;

    // Sync periodically
    if (entry_count_ % JOURNAL_SYNC_INTERVAL == 0) {
        sync();
    }

    // Rotate if needed
    return rotateIfNeeded();
}

esp_err_t JournalWriter::writeDelete(size_t position, size_t length) {
    if (!current_file_) {
        return ESP_ERR_INVALID_STATE;
    }

    JournalOpType type = JournalOpType::DELETE;
    uint32_t pos = static_cast<uint32_t>(position);
    uint32_t len = static_cast<uint32_t>(length);
    uint64_t timestamp = esp_timer_get_time();

    fwrite(&type, sizeof(JournalOpType), 1, current_file_);
    fwrite(&pos, sizeof(uint32_t), 1, current_file_);
    fwrite(&len, sizeof(uint32_t), 1, current_file_);
    fwrite(&timestamp, sizeof(uint64_t), 1, current_file_);

    size_t entry_size = sizeof(JournalOpType) + sizeof(uint32_t) * 2 + sizeof(uint64_t);
    file_size_ += entry_size;
    entry_count_++;

    if (entry_count_ % JOURNAL_SYNC_INTERVAL == 0) {
        sync();
    }

    return rotateIfNeeded();
}

esp_err_t JournalWriter::writeMarker() {
    if (!current_file_) {
        return ESP_ERR_INVALID_STATE;
    }

    JournalOpType type = JournalOpType::MARKER;
    uint64_t timestamp = esp_timer_get_time();

    fwrite(&type, sizeof(JournalOpType), 1, current_file_);
    uint32_t zero = 0;
    fwrite(&zero, sizeof(uint32_t), 2, current_file_);  // position and length (unused)
    fwrite(&timestamp, sizeof(uint64_t), 1, current_file_);

    sync();
    return ESP_OK;
}

esp_err_t JournalWriter::sync() {
    if (current_file_) {
        fflush(current_file_);
        fsync(fileno(current_file_));
    }
    return ESP_OK;
}

std::string JournalWriter::getCurrentPath() const {
    return getJournalFilePath(project_id_, file_index_);
}

// ============================================================================
// JournalReader Implementation
// ============================================================================

JournalReader::JournalReader(const std::string& project_id)
    : project_id_(project_id)
    , current_file_(nullptr)
    , file_index_(0)
    , ops_read_(0)
{
}

JournalReader::~JournalReader() {
    closeFile();
}

esp_err_t JournalReader::init() {
    return openOldestFile();
}

esp_err_t JournalReader::openOldestFile() {
    closeFile();

    std::string journal_dir = getRecoveryJournalDir(project_id_);

    // Find the oldest journal file (lowest index)
    int oldest_index = -1;
    DIR* dir = opendir(journal_dir.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strncmp(entry->d_name, "edit_", 5) == 0) {
                int idx = atoi(entry->d_name + 5);
                if (oldest_index == -1 || idx < oldest_index) {
                    oldest_index = idx;
                }
            }
        }
        closedir(dir);
    }

    if (oldest_index == -1) {
        return ESP_ERR_NOT_FOUND;
    }

    file_index_ = oldest_index;
    std::string path = getJournalFilePath(project_id_, file_index_);
    current_file_ = fopen(path.c_str(), "rb");
    if (!current_file_) {
        ESP_LOGE(TAG, "Failed to open journal file: %s", path.c_str());
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Opened journal for reading: %s", path.c_str());
    return ESP_OK;
}

esp_err_t JournalReader::openNextFile() {
    closeFile();
    file_index_++;
    std::string path = getJournalFilePath(project_id_, file_index_);
    current_file_ = fopen(path.c_str(), "rb");
    return current_file_ ? ESP_OK : ESP_ERR_NOT_FOUND;
}

void JournalReader::closeFile() {
    if (current_file_) {
        fclose(current_file_);
        current_file_ = nullptr;
    }
}

bool JournalReader::hasJournals() const {
    return current_file_ != nullptr;
}

size_t JournalReader::getOperationCount() const {
    // Count operations in all journal files
    size_t count = 0;
    std::string journal_dir = getRecoveryJournalDir(project_id_);
    DIR* dir = opendir(journal_dir.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strncmp(entry->d_name, "edit_", 5) == 0) {
                std::string path = journal_dir + "/" + entry->d_name;
                FILE* f = fopen(path.c_str(), "rb");
                if (f) {
                    while (!feof(f)) {
                        JournalOpType type;
                        if (fread(&type, sizeof(JournalOpType), 1, f) == 1) {
                            uint32_t pos, len;
                            fread(&pos, sizeof(uint32_t), 1, f);
                            fread(&len, sizeof(uint32_t), 1, f);
                            uint64_t timestamp;
                            fread(&timestamp, sizeof(uint64_t), 1, f);

                            if (type == JournalOpType::INSERT) {
                                fseek(f, len, SEEK_CUR);
                            }
                            count++;
                        }
                    }
                    fclose(f);
                }
            }
        }
        closedir(dir);
    }
    return count;
}

std::unique_ptr<JournalEntry> JournalReader::readNext() {
    if (!current_file_) {
        return nullptr;
    }

    JournalOpType type;
    if (fread(&type, sizeof(JournalOpType), 1, current_file_) != 1) {
        // End of file, try next file
        if (openNextFile() == ESP_OK) {
            return readNext();
        }
        return nullptr;
    }

    uint32_t position, length;
    fread(&position, sizeof(uint32_t), 1, current_file_);
    fread(&length, sizeof(uint32_t), 1, current_file_);

    uint64_t timestamp;
    fread(&timestamp, sizeof(uint64_t), 1, current_file_);

    // Allocate entry with space for data
    size_t entry_size = sizeof(JournalEntry) - 1 + (type == JournalOpType::INSERT ? length : 0);
    JournalEntry* entry = (JournalEntry*)malloc(entry_size);

    entry->type = type;
    entry->position = position;
    entry->length = length;
    entry->timestamp = timestamp;

    if (type == JournalOpType::INSERT && length > 0) {
        fread(entry->data, 1, length, current_file_);
    }

    ops_read_++;
    return std::unique_ptr<JournalEntry>(entry);
}

std::string JournalReader::replay() {
    std::string content;

    // First, try to load from autosave.tmp as base
    std::string autosave_path = getAutosaveTempPath(project_id_);
    FILE* f = fopen(autosave_path.c_str(), "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        content.resize(size);
        fread(&content[0], 1, size, f);
        fclose(f);
        ESP_LOGI(TAG, "Loaded autosave base: %zu bytes", content.size());
    }

    // Replay journal entries
    size_t ops_applied = 0;
    while (auto entry = readNext()) {
        switch (entry->type) {
            case JournalOpType::INSERT:
                if (entry->position <= content.length()) {
                    content.insert(entry->position, std::string(entry->data, entry->length));
                    ops_applied++;
                }
                break;

            case JournalOpType::DELETE:
                if (entry->position + entry->length <= content.length()) {
                    content.erase(entry->position, entry->length);
                    ops_applied++;
                }
                break;

            case JournalOpType::MARKER:
                // Just a checkpoint, ignore
                break;
        }
    }

    ESP_LOGI(TAG, "Replayed %zu journal operations, final content: %zu bytes", ops_applied, content.size());
    return content;
}

esp_err_t JournalReader::cleanup() {
    closeFile();

    std::string journal_dir = getRecoveryJournalDir(project_id_);
    DIR* dir = opendir(journal_dir.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                std::string path = journal_dir + "/" + entry->d_name;
                unlink(path.c_str());
                ESP_LOGI(TAG, "Deleted journal file: %s", entry->d_name);
            }
        }
        closedir(dir);
        rmdir(journal_dir.c_str());
    }

    return ESP_OK;
}

// ============================================================================
// RecoveryManager Implementation
// ============================================================================

std::string RecoveryManager::recover(const std::string& project_id) {
    ESP_LOGI(TAG, "Starting recovery for project: %s", project_id.c_str());

    // Try journal-based recovery first
    JournalReader reader(project_id);
    if (reader.init() == ESP_OK && reader.hasJournals()) {
        ESP_LOGI(TAG, "Using journal-based recovery");
        std::string content = reader.replay();
        reader.cleanup();
        return content;
    }

    // Fall back to autosave.tmp only
    ESP_LOGI(TAG, "Using autosave-based recovery");
    return readRecoveredContentFromAutosave(project_id);
}

esp_err_t RecoveryManager::cleanup(const std::string& project_id) {
    // Delete autosave.tmp
    std::string autosave_path = getAutosaveTempPath(project_id);
    unlink(autosave_path.c_str());

    // Delete journal files
    JournalReader reader(project_id);
    if (reader.init() == ESP_OK) {
        reader.cleanup();
    }

    ESP_LOGI(TAG, "Cleaned up recovery files for project: %s", project_id.c_str());
    return ESP_OK;
}

// ============================================================================
// Legacy Functions (Backward Compatibility)
// ============================================================================

bool checkRecoveryNeeded(const std::string& project_id) {
    if (project_id.empty()) {
        return false;
    }

    // Check for autosave.tmp
    std::string autosave_path = getAutosaveTempPath(project_id);
    struct stat st;
    if (stat(autosave_path.c_str(), &st) == 0) {
        ESP_LOGI(TAG, "Found autosave.tmp - recovery needed");
        return true;
    }

    // Check for recovery journal
    std::string journal_dir = getRecoveryJournalDir(project_id);
    DIR* dir = opendir(journal_dir.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                ESP_LOGI(TAG, "Found recovery journal - recovery needed");
                closedir(dir);
                return true;
            }
        }
        closedir(dir);
    }

    return false;
}

std::string readRecoveredContentFromAutosave(const std::string& project_id) {
    std::string autosave_path = getAutosaveTempPath(project_id);
    FILE* f = fopen(autosave_path.c_str(), "r");
    if (!f) {
        ESP_LOGW(TAG, "Could not open autosave.tmp for reading");
        return "";
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Read content
    std::string content;
    content.resize(size);
    size_t read = fread(&content[0], 1, size, f);
    content.resize(read);
    fclose(f);

    ESP_LOGI(TAG, "Read %zu bytes from autosave.tmp", read);
    return content;
}
