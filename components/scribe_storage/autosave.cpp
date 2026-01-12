#include "autosave.h"
#include "storage_manager.h"
#include <esp_log.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

static const char* TAG = "SCRIBE_AUTOSAVE";

AutosaveManager& AutosaveManager::getInstance() {
    static AutosaveManager instance;
    return instance;
}

esp_err_t AutosaveManager::init() {
    ESP_LOGI(TAG, "Autosave manager initialized");
    return ESP_OK;
}

esp_err_t AutosaveManager::queueSave(const DocSnapshot& snapshot, SaveCallback callback) {
    esp_err_t ret = performSave(snapshot);
    if (callback) {
        callback(ret);
    }
    return ret;
}

esp_err_t AutosaveManager::saveNow(const DocSnapshot& snapshot) {
    ESP_LOGI(TAG, "Manual save for project %s", snapshot.project_id.c_str());
    esp_err_t ret = performSave(snapshot);

    // Create snapshot on manual save
    if (ret == ESP_OK) {
        std::string project_path = std::string(SCRIBE_PROJECTS_DIR) + "/" + snapshot.project_id;
        createSnapshot(project_path, snapshot.content);
    }

    return ret;
}

esp_err_t AutosaveManager::performSave(const DocSnapshot& snapshot) {
    saving_ = true;
    std::string project_path = std::string(SCRIBE_PROJECTS_DIR) + "/" + snapshot.project_id;

    esp_err_t ret = atomicSave(project_path, snapshot.content);
    saving_ = false;

    return ret;
}

esp_err_t AutosaveManager::atomicSave(const std::string& project_path, const std::string& content) {
    struct stat st;
    if (stat(project_path.c_str(), &st) != 0) {
        mkdir(project_path.c_str(), 0755);
    }

    // Write to temporary file
    std::string tmp_path = project_path + "/autosave.tmp";
    FILE* f = fopen(tmp_path.c_str(), "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create autosave.tmp");
        return ESP_FAIL;
    }

    fwrite(content.c_str(), 1, content.size(), f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);

    // Atomic rename
    std::string final_path = project_path + "/manuscript.md";
    if (rename(tmp_path.c_str(), final_path.c_str()) != 0) {
        ESP_LOGE(TAG, "Failed to rename autosave.tmp to manuscript.md");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Saved manuscript.md (%zu bytes)", content.size());
    return ESP_OK;
}

esp_err_t AutosaveManager::createSnapshot(const std::string& project_path, const std::string& content) {
    (void)content;
    std::string snapshot_dir = project_path + "/snapshots";
    struct stat st;
    if (stat(snapshot_dir.c_str(), &st) != 0) {
        mkdir(snapshot_dir.c_str(), 0755);
    }

    // Rotate snapshots: ~3 -> delete, ~2 -> ~3, ~1 -> ~2, current -> ~1
    std::string snap3 = snapshot_dir + "/manuscript.md.~3";
    std::string snap2 = snapshot_dir + "/manuscript.md.~2";
    std::string snap1 = snapshot_dir + "/manuscript.md.~1";

    // Delete oldest
    unlink(snap3.c_str());

    // Rotate
    rename(snap2.c_str(), snap3.c_str());
    rename(snap1.c_str(), snap2.c_str());

    // Create new snapshot
    std::string final_path = project_path + "/manuscript.md";
    std::ifstream src(final_path, std::ios::binary);
    if (!src.is_open()) {
        ESP_LOGW(TAG, "No manuscript to snapshot");
        return ESP_ERR_NOT_FOUND;
    }
    std::ofstream dst(snap1, std::ios::binary);
    if (!dst.is_open()) {
        ESP_LOGE(TAG, "Failed to write snapshot");
        return ESP_FAIL;
    }
    dst << src.rdbuf();

    ESP_LOGI(TAG, "Created snapshot %s", snap1.c_str());
    return ESP_OK;
}
