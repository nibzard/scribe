#include "export_sd.h"
#include "../scribe_storage/storage_manager.h"
#include <esp_log.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>
#include <unistd.h>

static const char* TAG = "SCRIBE_EXPORT_SD";

// Export directory on SD card
#define EXPORT_DIR "/sdcard/Scribe/Exports"

// Export to SD card as .txt or .md
esp_err_t exportToSD(const std::string& project_path, const std::string& content, const std::string& extension) {
    // Create export directory if it doesn't exist
    struct stat st;
    if (stat(EXPORT_DIR, &st) != 0) {
        mkdir(EXPORT_DIR, 0755);
    }

    // Extract project name from path
    std::string trimmed_path = project_path;
    while (!trimmed_path.empty() && trimmed_path.back() == '/') {
        trimmed_path.pop_back();
    }
    size_t last_slash = trimmed_path.find_last_of('/');
    std::string project_name = (!trimmed_path.empty() && last_slash != std::string::npos)
        ? trimmed_path.substr(last_slash + 1)
        : trimmed_path;
    if (project_name.empty()) {
        project_name = "untitled";
    }

    // Create export filename with timestamp
    // Format: PROJECT_NAME_YYYYMMDD_HHMMSS.EXT
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);

    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%s_%s%s",
             EXPORT_DIR, project_name.c_str(), timestamp, extension.c_str());

    // Write content to file
    FILE* f = fopen(filename, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create export file: %s", filename);
        return ESP_FAIL;
    }

    size_t written = fwrite(content.c_str(), 1, content.length(), f);
    fclose(f);

    if (written != content.length()) {
        ESP_LOGE(TAG, "Failed to write complete file (%zu/%zu)", written, content.length());
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Exported to %s (%zu bytes)", filename, content.length());
    return ESP_OK;
}

// List all exports in the export directory
std::vector<std::string> listExports() {
    std::vector<std::string> exports;

    DIR* dir = opendir(EXPORT_DIR);
    if (!dir) {
        ESP_LOGW(TAG, "No export directory found");
        return exports;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            exports.push_back(std::string(entry->d_name));
        }
    }

    closedir(dir);
    return exports;
}

// Delete an export file
esp_err_t deleteExport(const std::string& filename) {
    std::string path = std::string(EXPORT_DIR) + "/" + filename;

    if (unlink(path.c_str()) == 0) {
        ESP_LOGI(TAG, "Deleted export: %s", filename.c_str());
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to delete export: %s", filename.c_str());
    return ESP_FAIL;
}
