#include "autosave.h"
#include "storage_manager.h"
#include <esp_log.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>
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
        createSnapshot(project_path);
    }

    return ret;
}

esp_err_t AutosaveManager::performSave(const DocSnapshot& snapshot) {
    saving_ = true;
    StorageManager& storage = StorageManager::getInstance();
    esp_err_t dir_ret = storage.ensureDirectories();
    if (dir_ret != ESP_OK) {
        ESP_LOGE(TAG, "Storage dirs unavailable: %s", esp_err_to_name(dir_ret));
        saving_ = false;
        return dir_ret;
    }
    std::string project_path = std::string(SCRIBE_PROJECTS_DIR) + "/" + snapshot.project_id;

    esp_err_t ret = atomicSave(project_path, snapshot.table);
    saving_ = false;

    return ret;
}

esp_err_t AutosaveManager::atomicSave(const std::string& project_path, const PieceTableSnapshot& table) {
    struct stat st;
    if (stat(project_path.c_str(), &st) != 0) {
        if (mkdir(project_path.c_str(), 0755) != 0 && errno != EEXIST) {
            ESP_LOGE(TAG, "Failed to create project dir %s: errno %d (%s)",
                     project_path.c_str(), errno, strerror(errno));
            return ESP_FAIL;
        }
    }

    // Write to temporary file
    std::string tmp_path = project_path + "/autosave.tmp";
    FILE* f = fopen(tmp_path.c_str(), "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s: errno %d (%s)", tmp_path.c_str(), errno, strerror(errno));
        return ESP_FAIL;
    }

    esp_err_t write_ret = writeSnapshot(f, table);
    fflush(f);
    if (fsync(fileno(f)) != 0) {
        ESP_LOGW(TAG, "fsync failed for %s: errno %d (%s)", tmp_path.c_str(), errno, strerror(errno));
    }
    fclose(f);

    if (write_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write snapshot to %s: %s", tmp_path.c_str(), esp_err_to_name(write_ret));
        return write_ret;
    }

    // Atomic rename
    std::string final_path = project_path + "/manuscript.md";
    if (rename(tmp_path.c_str(), final_path.c_str()) != 0) {
        ESP_LOGE(TAG, "Failed to rename %s -> %s: errno %d (%s)",
                 tmp_path.c_str(), final_path.c_str(), errno, strerror(errno));
        unlink(tmp_path.c_str());
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Saved manuscript.md");
    return ESP_OK;
}

esp_err_t AutosaveManager::writeSnapshot(FILE* f, const PieceTableSnapshot& table) {
    if (!f || !table.original_buffer || !table.add_buffer) {
        return ESP_ERR_INVALID_ARG;
    }

    const std::string& original = *table.original_buffer;
    const std::string& add = *table.add_buffer;

    for (const auto& piece : table.pieces) {
        const std::string& buffer = (piece.type == Piece::Type::ORIGINAL) ? original : add;
        if (piece.start + piece.length > buffer.size()) {
            ESP_LOGE(TAG, "Snapshot piece out of bounds");
            return ESP_FAIL;
        }

        if (piece.length > 0) {
            size_t written = fwrite(buffer.data() + piece.start, 1, piece.length, f);
            if (written != piece.length) {
                ESP_LOGE(TAG, "Failed to write snapshot data: wrote %zu/%zu errno %d (%s)",
                         written, piece.length, errno, strerror(errno));
                return ESP_FAIL;
            }
        }
    }

    return ESP_OK;
}

esp_err_t AutosaveManager::createSnapshot(const std::string& project_path) {
    std::string snapshot_dir = project_path + "/snapshots";
    struct stat st;
    if (stat(snapshot_dir.c_str(), &st) != 0) {
        if (mkdir(snapshot_dir.c_str(), 0755) != 0 && errno != EEXIST) {
            ESP_LOGE(TAG, "Failed to create snapshot dir %s: errno %d (%s)",
                     snapshot_dir.c_str(), errno, strerror(errno));
            return ESP_FAIL;
        }
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
        ESP_LOGE(TAG, "Failed to write snapshot %s: errno %d (%s)",
                 snap1.c_str(), errno, strerror(errno));
        return ESP_FAIL;
    }
    dst << src.rdbuf();

    ESP_LOGI(TAG, "Created snapshot %s", snap1.c_str());
    return ESP_OK;
}
