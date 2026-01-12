#pragma once

#include <esp_err.h>
#include <string>
#include <functional>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// AI suggestion style
enum class AIStyle {
    CONTINUE,      // Continue writing
    REWRITE,       // Rewrite/paraphrase
    SUMMARIZE,     // Summarize
    EXPAND,        // Expand/elaborate
    CUSTOM         // Custom instruction
};

// Streaming callback for text delta
using StreamCallback = std::function<void(const char* delta)>;

// Completion callback
using CompleteCallback = std::function<void(bool success, const std::string& error)>;

// Progress callback
using ProgressCallback = std::function<void(const std::string& status)>;

// AI assistance using OpenAI Responses API with streaming
// Based on SPECS.md section 6.9 - OpenAI Responses API
// Offline-first: never blocks typing, shows clear errors
class AIAssist {
public:
    static AIAssist& getInstance();

    // Initialize AI service
    esp_err_t init();

    // Check if AI is enabled
    bool isEnabled() const { return enabled_.load(); }

    // Enable/disable AI
    void setEnabled(bool enabled) { enabled_.store(enabled); }

    // Set API key (stored in NVS)
    esp_err_t setAPIKey(const std::string& key);

    // Get stored API key
    std::string getAPIKey() const { return api_key_; }

    // Generate suggestion for selected text
    esp_err_t generateSuggestion(const std::string& text, AIStyle style,
                                  const std::string& custom_instruction,
                                  StreamCallback stream_cb, CompleteCallback complete_cb);

    // Cancel ongoing request
    void cancel();

    // Check if currently generating
    bool isGenerating() const { return generating_.load(); }

    // Register progress callback
    void setProgressCallback(ProgressCallback cb) { progress_cb_ = cb; }

    // Get style description
    static const char* getStyleDescription(AIStyle style);

private:
    AIAssist() = default;
    ~AIAssist() = default;

    std::atomic<bool> enabled_{false};
    std::atomic<bool> generating_{false};
    std::string api_key_;

    StreamCallback stream_cb_;
    CompleteCallback complete_cb_;
    ProgressCallback progress_cb_;

    // Task handle for background generation
    TaskHandle_t gen_task_{nullptr};
    SemaphoreHandle_t mutex_{nullptr};

    // Cancel flag
    std::atomic<bool> cancel_flag_{false};

    // Background task for AI generation
    static void generationTask(void* arg);

    // Build request for OpenAI API
    std::string buildRequest(const std::string& text, AIStyle style,
                            const std::string& custom_instruction);

    // Build system prompt based on style
    std::string buildSystemPrompt(AIStyle style, const std::string& custom_instruction);

    // Parse SSE (Server-Sent Events) data
    void parseSSE(const std::string& data);

    // Load API key from NVS
    esp_err_t loadAPIKey();

    // Save API key to NVS
    esp_err_t saveAPIKey();

    // HTTP client configuration
    struct RequestData {
        std::string text;
        AIStyle style;
        std::string custom_instruction;
        std::string api_key;
    };
};
