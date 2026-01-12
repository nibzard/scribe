#include "ai_assist.h"
#include "../scribe_secrets/secrets_nvs.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <string>

static const char* TAG = "SCRIBE_AI_ASSIST";

AIAssist& AIAssist::getInstance() {
    static AIAssist instance;
    return instance;
}

esp_err_t AIAssist::init() {
    ESP_LOGI(TAG, "Initializing AI assistance service");

    // Load API key from secrets if available
    SecretsNVS& secrets = SecretsNVS::getInstance();
    secrets.init();

    std::string key = secrets.getOpenAIKey();
    if (!key.empty()) {
        api_key_ = key;
        ESP_LOGI(TAG, "Loaded API key from NVS");
    }

    return ESP_OK;
}

esp_err_t AIAssist::generateSuggestion(const std::string& text, const std::string& instruction,
                                       StreamCallback stream_cb, CompleteCallback complete_cb) {
    if (!enabled_) {
        ESP_LOGW(TAG, "AI is disabled");
        if (complete_cb) {
            complete_cb(false);
        }
        return ESP_ERR_INVALID_STATE;
    }

    if (api_key_.empty()) {
        ESP_LOGE(TAG, "No API key configured");
        if (complete_cb) {
            complete_cb(false);
        }
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Generating AI suggestion for %zu characters", text.length());

    // TODO: Implement HTTP request to OpenAI Responses API
    // TODO: Implement SSE parsing for streaming responses
    // TODO: Call stream_cb with text deltas

    ESP_LOGW(TAG, "AI generation not yet implemented - returning stub");
    if (complete_cb) {
        complete_cb(true);  // Placeholder
    }

    return ESP_ERR_NOT_SUPPORTED;
}

void AIAssist::cancel() {
    ESP_LOGI(TAG, "Cancelling AI request");
    // TODO: Cancel ongoing HTTP request
}
