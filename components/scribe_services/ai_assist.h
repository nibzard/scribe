#pragma once

#include <esp_err.h>
#include <string>
#include <functional>

// AI assistance stub - MVP4 feature
// Based on SPECS.md section 6.9 - OpenAI Responses API with streaming

class AIAssist {
public:
    // Streaming callback for text delta
    using StreamCallback = std::function<void(const char* delta)>;

    // Completion callback
    using CompleteCallback = std::function<void(bool success)>;

    static AIAssist& getInstance();

    // Initialize AI service
    esp_err_t init();

    // Check if AI is enabled
    bool isEnabled() const { return enabled_; }

    // Enable/disable AI
    void setEnabled(bool enabled) { enabled_ = enabled; }

    // Set API key
    void setAPIKey(const std::string& key) { api_key_ = key; }

    // Generate suggestion for selected text
    esp_err_t generateSuggestion(const std::string& text, const std::string& instruction,
                                  StreamCallback stream_cb, CompleteCallback complete_cb);

    // Cancel ongoing request
    void cancel();

private:
    AIAssist() : enabled_(false) {}
    ~AIAssist() = default;

    bool enabled_;
    std::string api_key_;
};
