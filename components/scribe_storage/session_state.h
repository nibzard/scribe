#pragma once

#include <esp_err.h>
#include <string>
#include <cstdint>

// Session state - persists editor state between boots
// Implements SPECS.md: "Always return to the last sentence"

struct EditorState {
    size_t cursor_pos = 0;
    size_t scroll_offset = 0;
    uint64_t last_edit_time = 0;
};

class SessionState {
public:
    static SessionState& getInstance();

    // Initialize session state storage
    esp_err_t init();

    // Save editor state for current project
    esp_err_t saveEditorState(const std::string& project_id, const EditorState& state);

    // Load editor state for current project
    esp_err_t loadEditorState(const std::string& project_id, EditorState& state);

    // Clear editor state for a project
    esp_err_t clearEditorState(const std::string& project_id);

private:
    SessionState() = default;
    ~SessionState() = default;

    std::string getStatePath(const std::string& project_id);
};
