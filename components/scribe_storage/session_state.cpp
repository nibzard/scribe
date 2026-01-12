#include "session_state.h"
#include "storage_manager.h"
#include <esp_log.h>
#include <sys/stat.h>
#include <cstdio>
#include <unistd.h>

static const char* TAG = "SCRIBE_SESSION_STATE";

SessionState& SessionState::getInstance() {
    static SessionState instance;
    return instance;
}

esp_err_t SessionState::init() {
    ESP_LOGI(TAG, "Initializing session state storage");
    return ESP_OK;
}

std::string SessionState::getStatePath(const std::string& project_id) {
    return std::string(SCRIBE_LOGS_DIR) + "/state_" + project_id + ".json";
}

esp_err_t SessionState::saveEditorState(const std::string& project_id, const EditorState& state) {
    std::string path = getStatePath(project_id);

    FILE* f = fopen(path.c_str(), "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create state file: %s", path.c_str());
        return ESP_FAIL;
    }

    // Write as simple JSON
    fprintf(f, "{\n");
    fprintf(f, "  \"cursor_pos\": %zu,\n", state.cursor_pos);
    fprintf(f, "  \"scroll_offset\": %zu,\n", state.scroll_offset);
    fprintf(f, "  \"last_edit_time\": %llu\n", (unsigned long long)state.last_edit_time);
    fprintf(f, "}\n");

    fflush(f);
    fsync(fileno(f));
    fclose(f);

    ESP_LOGD(TAG, "Saved state for project %s: cursor=%zu", project_id.c_str(), state.cursor_pos);
    return ESP_OK;
}

esp_err_t SessionState::loadEditorState(const std::string& project_id, EditorState& state) {
    std::string path = getStatePath(project_id);

    FILE* f = fopen(path.c_str(), "r");
    if (!f) {
        ESP_LOGD(TAG, "No state file found for project %s", project_id.c_str());
        return ESP_ERR_NOT_FOUND;
    }

    // Simple JSON parsing
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        // Parse cursor_pos
        if (strstr(line, "\"cursor_pos\"")) {
            sscanf(line, "  \"cursor_pos\": %zu,", &state.cursor_pos);
        }
        // Parse scroll_offset
        else if (strstr(line, "\"scroll_offset\"")) {
            sscanf(line, "  \"scroll_offset\": %zu,", &state.scroll_offset);
        }
        // Parse last_edit_time
        else if (strstr(line, "\"last_edit_time\"")) {
            sscanf(line, "  \"last_edit_time\": %llu", (unsigned long long*)&state.last_edit_time);
        }
    }

    fclose(f);
    ESP_LOGI(TAG, "Loaded state for project %s: cursor=%zu", project_id.c_str(), state.cursor_pos);
    return ESP_OK;
}

esp_err_t SessionState::clearEditorState(const std::string& project_id) {
    std::string path = getStatePath(project_id);

    if (unlink(path.c_str()) == 0) {
        ESP_LOGI(TAG, "Cleared state for project %s", project_id.c_str());
        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}
