#include "ai_assist.h"
#include "../scribe_secrets/secrets_nvs.h"
#include "../scribe_services/wifi_manager.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <cstring>

static const char* TAG = "SCRIBE_AI";

// OpenAI API endpoint
#define OPENAI_API_URL "https://api.openai.com/v1/chat/completions"

// HTTP buffer size
#define HTTP_BUFFER_SIZE 2048

// HTTP event handler for streaming
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    AIAssist* ai = static_cast<AIAssist*>(evt->user_data);

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                std::string chunk((char*)evt->data, evt->data_len);
                ai->parseSSE(chunk);
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

AIAssist& AIAssist::getInstance() {
    static AIAssist instance;
    return instance;
}

esp_err_t AIAssist::init() {
    ESP_LOGI(TAG, "Initializing AI assistance service");

    // Create mutex
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Load API key from secrets
    loadAPIKey();

    if (!api_key_.empty()) {
        ESP_LOGI(TAG, "API key loaded (redacted)");
    } else {
        ESP_LOGI(TAG, "No API key configured - AI features disabled");
    }

    return ESP_OK;
}

esp_err_t AIAssist::setAPIKey(const std::string& key) {
    if (key.empty()) {
        ESP_LOGE(TAG, "API key cannot be empty");
        return ESP_ERR_INVALID_ARG;
    }

    // Basic validation: OpenAI keys start with "sk-" and are ~51 chars
    if (key.length() < 20) {
        ESP_LOGE(TAG, "API key too short");
        return ESP_ERR_INVALID_ARG;
    }

    api_key_ = key;
    return saveAPIKey();
}

esp_err_t AIAssist::generateSuggestion(const std::string& text, AIStyle style,
                                       const std::string& custom_instruction,
                                       StreamCallback stream_cb, CompleteCallback complete_cb) {
    if (!enabled_.load()) {
        ESP_LOGW(TAG, "AI is disabled");
        if (complete_cb) {
            complete_cb(false, "AI is disabled");
        }
        return ESP_ERR_INVALID_STATE;
    }

    if (api_key_.empty()) {
        ESP_LOGE(TAG, "No API key configured");
        if (complete_cb) {
            complete_cb(false, "No API key configured");
        }
        return ESP_ERR_INVALID_STATE;
    }

    if (!WiFiManager::getInstance().isConnected()) {
        ESP_LOGW(TAG, "Not connected to WiFi");
        if (complete_cb) {
            complete_cb(false, "Offline - AI requires internet");
        }
        return ESP_ERR_INVALID_STATE;
    }

    if (generating_.load()) {
        ESP_LOGW(TAG, "Already generating");
        return ESP_ERR_INVALID_STATE;
    }

    if (text.empty()) {
        ESP_LOGE(TAG, "Text is empty");
        return ESP_ERR_INVALID_ARG;
    }

    // Store callbacks
    stream_cb_ = stream_cb;
    complete_cb_ = complete_cb;

    // Reset cancel flag
    cancel_flag_.store(false);

    // Create request data
    RequestData* req_data = new RequestData{
        .text = text,
        .style = style,
        .custom_instruction = custom_instruction,
        .api_key = api_key_
    };

    // Create background task
    generating_.store(true);

    if (progress_cb_) {
        progress_cb_("Connecting to AI...");
    }

    BaseType_t ret = xTaskCreate(generationTask, "ai_gen", 8192, req_data, 5, &gen_task_);
    if (ret != pdPASS) {
        generating_.store(false);
        delete req_data;
        ESP_LOGE(TAG, "Failed to create generation task");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t AIAssist::requestSuggestion(AIStyle style, const std::string& text,
                                      StreamCallback stream_cb, CompleteCallback complete_cb) {
    return generateSuggestion(text, style, "", stream_cb, complete_cb);
}

void AIAssist::cancel() {
    if (generating_.load()) {
        ESP_LOGI(TAG, "Cancelling AI generation...");
        cancel_flag_.store(true);
    }
}

void AIAssist::generationTask(void* arg) {
    RequestData* req = static_cast<RequestData*>(arg);
    AIAssist* ai = &AIAssist::getInstance();

    // Build JSON request
    std::string json_body = ai->buildRequest(req->text, req->style, req->custom_instruction);

    // Configure HTTP client
    esp_http_client_config_t config = {};
    config.url = OPENAI_API_URL;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 60000;  // 60 second timeout
    config.buffer_size = HTTP_BUFFER_SIZE;
    config.user_data = ai;
    config.event_handler = http_event_handler;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", (std::string("Bearer ") + req->api_key).c_str());

    // Set POST body
    esp_http_client_set_post_field(client, json_body.c_str(), json_body.length());

    // Notify progress
    if (ai->progress_cb_) {
        ai->progress_cb_("Generating suggestion...");
    }

    // Perform request
    esp_err_t ret = esp_http_client_perform(client);

    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    // Handle result
    if (ai->cancel_flag_.load()) {
        if (ai->complete_cb_) {
            ai->complete_cb_(false, "Cancelled");
        }
        ESP_LOGI(TAG, "Generation cancelled");
    } else if (ret == ESP_OK) {
        if (status == 200) {
            if (ai->complete_cb_) {
                ai->complete_cb_(true, "");
            }
            ESP_LOGI(TAG, "Generation complete");
        } else {
            char err_buf[64];
            snprintf(err_buf, sizeof(err_buf), "API error: HTTP %d", status);
            if (ai->complete_cb_) {
                ai->complete_cb_(false, err_buf);
            }
            ESP_LOGW(TAG, "API returned status %d", status);
        }
    } else {
        char err_buf[64];
        snprintf(err_buf, sizeof(err_buf), "Network error: %s", esp_err_to_name(ret));
        if (ai->complete_cb_) {
            ai->complete_cb_(false, err_buf);
        }
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(ret));
    }

    ai->generating_.store(false);
    delete req;

    vTaskDelete(nullptr);
}

std::string AIAssist::buildRequest(const std::string& text, AIStyle style,
                                   const std::string& custom_instruction) {
    cJSON* root = cJSON_CreateObject();

    // Model
    cJSON_AddStringToObject(root, "model", "gpt-4o-mini");

    // System message
    std::string system_prompt = buildSystemPrompt(style, custom_instruction);
    cJSON* messages = cJSON_CreateArray();

    cJSON* system_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(system_msg, "role", "system");
    cJSON_AddStringToObject(system_msg, "content", system_prompt.c_str());
    cJSON_AddItemToArray(messages, system_msg);

    // User message with selected text
    cJSON* user_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(user_msg, "role", "user");
    cJSON_AddStringToObject(user_msg, "content", text.c_str());
    cJSON_AddItemToArray(messages, user_msg);

    cJSON_AddItemToObject(root, "messages", messages);

    // Streaming enabled
    cJSON_AddBoolToObject(root, "stream", true);

    // Temperature (0.7 for creativity)
    cJSON_AddNumberToObject(root, "temperature", 0.7);

    // Max tokens
    cJSON_AddNumberToObject(root, "max_tokens", 500);

    char* json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    std::string result(json_str);
    free(json_str);

    return result;
}

std::string AIAssist::buildSystemPrompt(AIStyle style, const std::string& custom_instruction) {
    switch (style) {
        case AIStyle::CONTINUE:
            return "You are a writing assistant. Continue the text naturally, maintaining "
                   "the same style, tone, and voice. Write 2-3 sentences that flow seamlessly "
                   "from the given text. Do not repeat the last sentence.";

        case AIStyle::REWRITE:
            return "You are a writing assistant. Rewrite the given text to improve clarity, "
                   "flow, and conciseness while preserving the original meaning. Maintain the "
                   "same tone and voice.";

        case AIStyle::SUMMARIZE:
            return "You are a writing assistant. Summarize the given text in 1-2 concise "
                   "sentences that capture the main points.";

        case AIStyle::EXPAND:
            return "You are a writing assistant. Expand on the given text by adding relevant "
                   "details, examples, or context. Maintain the same style and tone.";

        case AIStyle::CUSTOM:
            if (!custom_instruction.empty()) {
                return "You are a writing assistant. " + custom_instruction;
            }
            return "You are a helpful writing assistant.";

        default:
            return "You are a helpful writing assistant.";
    }
}

void AIAssist::parseSSE(const std::string& data) {
    if (cancel_flag_.load()) {
        return;
    }

    // SSE format: "data: {...}\n\n"
    size_t pos = 0;
    while ((pos = data.find("data: ", pos)) != std::string::npos) {
        pos += 6;  // Skip "data: "

        // Find end of line
        size_t end = data.find('\n', pos);
        if (end == std::string::npos) {
            end = data.find('\r', pos);
        }
        if (end == std::string::npos) {
            break;
        }

        std::string json_str = data.substr(pos, end - pos);

        // Check for [DONE] marker
        if (json_str == "[DONE]") {
            break;
        }

        // Parse JSON
        cJSON* root = cJSON_Parse(json_str.c_str());
        if (root) {
            // Extract delta content
            cJSON* choices = cJSON_GetObjectItem(root, "choices");
            if (choices && cJSON_IsArray(choices)) {
                cJSON* choice = cJSON_GetArrayItem(choices, 0);
                if (choice) {
                    cJSON* delta = cJSON_GetObjectItem(choice, "delta");
                    if (delta) {
                        cJSON* content = cJSON_GetObjectItem(delta, "content");
                        if (content && cJSON_IsString(content)) {
                            // Call stream callback with delta
                            if (stream_cb_) {
                                stream_cb_(content->valuestring);
                            }
                        }
                    }
                }
            }
            cJSON_Delete(root);
        }

        pos = end + 1;
    }
}

const char* AIAssist::getStyleDescription(AIStyle style) {
    switch (style) {
        case AIStyle::CONTINUE: return "Continue writing";
        case AIStyle::REWRITE: return "Rewrite";
        case AIStyle::SUMMARIZE: return "Summarize";
        case AIStyle::EXPAND: return "Expand";
        case AIStyle::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

esp_err_t AIAssist::loadAPIKey() {
    SecretsNVS& secrets = SecretsNVS::getInstance();
    secrets.init();
    api_key_ = secrets.getOpenAIKey();
    return ESP_OK;
}

esp_err_t AIAssist::saveAPIKey() {
    SecretsNVS& secrets = SecretsNVS::getInstance();
    secrets.init();
    return secrets.setOpenAIKey(api_key_);
}
