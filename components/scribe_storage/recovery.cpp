#include "recovery.h"
#include "storage_manager.h"
#include <esp_log.h>
#include <sys/stat.h>
#include <dirent.h>

static const char* TAG = "SCRIBE_RECOVERY";

std::string getAutosaveTempPath(const std::string& project_id) {
    return std::string(SCRIBE_PROJECTS_DIR) + "/" + project_id + "/autosave.tmp";
}

std::string getRecoveryJournalPath(const std::string& project_id) {
    return std::string(SCRIBE_PROJECTS_DIR) + "/" + project_id + "/journal";
}

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
    std::string journal_path = getRecoveryJournalPath(project_id);
    DIR* dir = opendir(journal_path.c_str());
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

std::string readRecoveredContent(const std::string& project_id) {
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
