#include "github_backup.h"
#include "wifi_manager.h"
#include "../scribe_secrets/secrets_nvs.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <mbedtls/base64.h>
#include <algorithm>
#include <cstring>

static const char* TAG = "SCRIBE_BACKUP";

// NVS keys for token storage
#define NVS_NAMESPACE "backup"
#define NVS_GITHUB_TOKEN_KEY "gh_token"

// GitHub API endpoints
#define GITHUB_API_BASE "https://api.github.com"
#define GITHUB_RAW_BASE "https://raw.githubusercontent.com"

// HTTP buffer size
#define HTTP_BUFFER_SIZE 4096

struct HttpResponseBuffer {
    char* data = nullptr;
    size_t size = 0;
    size_t length = 0;
};

static esp_err_t http_event_handler(esp_http_client_event_t* evt) {
    if (evt->event_id != HTTP_EVENT_ON_DATA || !evt->user_data) {
        return ESP_OK;
    }

    auto* buffer = static_cast<HttpResponseBuffer*>(evt->user_data);
    if (!buffer->data || buffer->size == 0) {
        return ESP_OK;
    }

    size_t remaining = buffer->size - 1 - buffer->length;
    if (remaining == 0) {
        return ESP_OK;
    }

    size_t to_copy = std::min(remaining, static_cast<size_t>(evt->data_len));
    if (to_copy > 0) {
        memcpy(buffer->data + buffer->length, evt->data, to_copy);
        buffer->length += to_copy;
        buffer->data[buffer->length] = '\0';
    }

    return ESP_OK;
}

static std::string base64Encode(const std::string& input) {
    if (input.empty()) {
        return "";
    }

    size_t out_len = ((input.size() + 2) / 3) * 4;
    std::string output(out_len, '\0');
    size_t written = 0;

    int ret = mbedtls_base64_encode(
        reinterpret_cast<unsigned char*>(&output[0]),
        output.size(),
        &written,
        reinterpret_cast<const unsigned char*>(input.data()),
        input.size()
    );

    if (ret != 0) {
        return "";
    }

    output.resize(written);
    return output;
}

GitHubBackup& GitHubBackup::getInstance() {
    static GitHubBackup instance;
    return instance;
}

esp_err_t GitHubBackup::init() {
    ESP_LOGI(TAG, "Initializing GitHub backup...");

    // Load token from NVS if exists
    loadToken();

    if (!token_.empty()) {
        ESP_LOGI(TAG, "GitHub token found (redacted)");
    } else {
        ESP_LOGI(TAG, "No GitHub token configured");
    }

    return ESP_OK;
}

esp_err_t GitHubBackup::setToken(const std::string& token) {
    if (token.empty()) {
        ESP_LOGE(TAG, "Token cannot be empty");
        return ESP_ERR_INVALID_ARG;
    }

    token_ = token;
    return saveToken();
}

std::string GitHubBackup::getToken() const {
    return token_;
}

esp_err_t GitHubBackup::configure(BackupProvider provider, const std::string& repo_owner,
                                   const std::string& repo_name, const std::string& branch) {
    provider_ = provider;
    repo_owner_ = repo_owner;
    repo_name_ = repo_name;
    branch_ = branch;

    ESP_LOGI(TAG, "Backup configured: %s/%s (branch: %s)",
             repo_owner.c_str(), repo_name.c_str(), branch.c_str());

    return ESP_OK;
}

esp_err_t GitHubBackup::queueBackup(const std::string& project_id, const std::string& content) {
    if (!enabled_) {
        return ESP_OK;  // Silently ignore if disabled
    }

    if (token_.empty()) {
        ESP_LOGW(TAG, "Cannot backup: no token configured");
        return ESP_ERR_INVALID_STATE;
    }

    // Check if already queued
    for (const auto& req : queue_) {
        if (req.project_id == project_id) {
            ESP_LOGD(TAG, "Project %s already in backup queue", project_id.c_str());
            return ESP_OK;
        }
    }

    BackupRequest req;
    req.project_id = project_id;
    req.content = content;
    req.last_sha = "";

    queue_.push_back(req);
    ESP_LOGI(TAG, "Queued backup for project %s", project_id.c_str());

    // Process immediately if online
    if (online_) {
        processQueue();
    }

    return ESP_OK;
}

esp_err_t GitHubBackup::syncNow() {
    if (!enabled_) {
        ESP_LOGW(TAG, "Backup is disabled");
        return ESP_ERR_INVALID_STATE;
    }

    if (token_.empty()) {
        ESP_LOGW(TAG, "No GitHub token configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (!online_) {
        ESP_LOGW(TAG, "Cannot sync: offline");
        return ESP_ERR_INVALID_STATE;
    }

    processQueue();
    return ESP_OK;
}

void GitHubBackup::setOnline(bool online) {
    bool was_online = online_;
    online_ = online;

    if (online && !was_online && !queue_.empty()) {
        ESP_LOGI(TAG, "Came online, processing %zu pending backups", queue_.size());
        processQueue();
    }
}

void GitHubBackup::processQueue() {
    if (queue_.empty()) {
        return;
    }

    status_ = BackupStatus::SYNCING;
    if (status_callback_) {
        status_callback_(BackupStatus::SYNCING, "Syncing to GitHub...");
    }

    // Process one item at a time to avoid blocking
    BackupRequest req = queue_.front();
    queue_.erase(queue_.begin());

    esp_err_t ret = ESP_FAIL;
    std::string path = getProjectPath(req.project_id);

    if (provider_ == BackupProvider::GITHUB_REPO) {
        ret = uploadToGitHub(path, req.content, "Scribe backup");
    } else {
        ret = uploadToGist("Scribe backup: " + req.project_id, req.content);
    }

    if (ret == ESP_OK) {
        status_ = BackupStatus::SUCCESS;
        if (status_callback_) {
            status_callback_(BackupStatus::SUCCESS, "Backup complete");
        }
        ESP_LOGI(TAG, "Backup successful for %s", req.project_id.c_str());
    } else {
        status_ = BackupStatus::FAILED;
        if (status_callback_) {
            status_callback_(BackupStatus::FAILED, "Backup failed");
        }
        ESP_LOGW(TAG, "Backup failed for %s: %s", req.project_id.c_str(), esp_err_to_name(ret));

        // Re-queue for retry
        queue_.push_back(req);
    }

    // Schedule next item if any
    if (!queue_.empty() && online_) {
        // TODO: Use timer to delay between uploads
    }
}

std::string GitHubBackup::getProjectPath(const std::string& project_id) const {
    // Path in repo: Projects/{project_id}/manuscript.md
    return "Projects/" + project_id + "/manuscript.md";
}

esp_err_t GitHubBackup::uploadToGitHub(const std::string& path, const std::string& content,
                                        const std::string& message) {
    if (token_.empty() || repo_owner_.empty() || repo_name_.empty()) {
        return ESP_ERR_INVALID_STATE;
    }

    // First, check if file exists and get its SHA
    std::string current_sha;
    esp_err_t ret = getCommitSHA(path, current_sha);

    // Build API URL
    char url[256];
    snprintf(url, sizeof(url), "%s/repos/%s/%s/contents/%s",
             GITHUB_API_BASE, repo_owner_.c_str(), repo_name_.c_str(), path.c_str());

    // Build request body
    std::string encoded = base64Encode(content);
    if (encoded.empty() && !content.empty()) {
        return ESP_ERR_INVALID_STATE;
    }

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "message", message.c_str());
    cJSON_AddStringToObject(json, "content", encoded.c_str());

    // Add SHA if updating existing file
    if (ret == ESP_OK && !current_sha.empty()) {
        cJSON_AddStringToObject(json, "sha", current_sha.c_str());
    }

    cJSON_AddStringToObject(json, "branch", branch_.c_str());

    char* json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    // HTTP response buffer
    char* response = (char*)malloc(HTTP_BUFFER_SIZE);
    if (!response) {
        free(json_str);
        return ESP_ERR_NO_MEM;
    }
    response[0] = '\0';
    HttpResponseBuffer response_buf{response, HTTP_BUFFER_SIZE, 0};

    // Configure HTTP client
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_PUT;
    config.timeout_ms = 30000;
    config.buffer_size = HTTP_BUFFER_SIZE;
    config.user_data = &response_buf;
    config.event_handler = http_event_handler;
    config.buffer_size_tx = 4096;

    // Add authorization header
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "token %s", token_.c_str());

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "User-Agent", "scribe-firmware");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_str, strlen(json_str));

    // Perform request
    ret = esp_http_client_perform(client);

    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    free(json_str);

    if (ret == ESP_OK) {
        if (status == 200 || status == 201) {
            ESP_LOGI(TAG, "Upload successful: HTTP %d", status);
            ret = ESP_OK;
        } else if (status == 409) {
            ESP_LOGW(TAG, "Conflict detected: HTTP %d", status);
            ret = ESP_ERR_INVALID_STATE;  // Conflict
        } else {
            ESP_LOGW(TAG, "Upload failed: HTTP %d", status);
            ret = ESP_FAIL;
        }
    }

    free(response);
    return ret;
}

esp_err_t GitHubBackup::uploadToGist(const std::string& description, const std::string& content) {
    if (token_.empty()) {
        return ESP_ERR_INVALID_STATE;
    }

    // Build Gist API request
    const char* url = "https://api.github.com/gists";

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "description", description.c_str());
    cJSON_AddBoolToObject(json, "public", false);  // Private gist

    cJSON* files = cJSON_CreateObject();
    cJSON* file = cJSON_CreateObject();
    cJSON_AddStringToObject(file, "content", content.c_str());
    cJSON_AddItemToObject(files, "manuscript.md", file);
    cJSON_AddItemToObject(json, "files", files);

    char* json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    // HTTP response buffer
    char* response = (char*)malloc(HTTP_BUFFER_SIZE);
    if (!response) {
        free(json_str);
        return ESP_ERR_NO_MEM;
    }
    response[0] = '\0';
    HttpResponseBuffer response_buf{response, HTTP_BUFFER_SIZE, 0};

    // Configure HTTP client
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 30000;
    config.buffer_size = HTTP_BUFFER_SIZE;
    config.user_data = &response_buf;
    config.event_handler = http_event_handler;

    // Add authorization header
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "token %s", token_.c_str());

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "User-Agent", "scribe-firmware");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_str, strlen(json_str));

    // Perform request
    esp_err_t ret = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    free(json_str);

    if (ret == ESP_OK && (status == 200 || status == 201)) {
        ESP_LOGI(TAG, "Gist created: HTTP %d", status);
        ret = ESP_OK;
    } else {
        ESP_LOGW(TAG, "Gist creation failed: HTTP %d", status);
        ret = ESP_FAIL;
    }

    free(response);
    return ret;
}

esp_err_t GitHubBackup::getCommitSHA(const std::string& path, std::string& out_sha) {
    if (token_.empty() || repo_owner_.empty() || repo_name_.empty()) {
        return ESP_ERR_INVALID_STATE;
    }

    // Build API URL
    char url[256];
    snprintf(url, sizeof(url), "%s/repos/%s/%s/contents/%s?ref=%s",
             GITHUB_API_BASE, repo_owner_.c_str(), repo_name_.c_str(),
             path.c_str(), branch_.c_str());

    // HTTP response buffer
    char* response = (char*)malloc(HTTP_BUFFER_SIZE);
    if (!response) {
        return ESP_ERR_NO_MEM;
    }
    response[0] = '\0';
    HttpResponseBuffer response_buf{response, HTTP_BUFFER_SIZE, 0};

    // Configure HTTP client
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 15000;
    config.buffer_size = HTTP_BUFFER_SIZE;
    config.user_data = &response_buf;
    config.event_handler = http_event_handler;

    // Add authorization header
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "token %s", token_.c_str());

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "User-Agent", "scribe-firmware");

    // Perform request
    esp_err_t ret = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (ret == ESP_OK && status == 200) {
        // Parse JSON to get SHA
        cJSON* root = cJSON_Parse(response);
        if (root) {
            cJSON* sha = cJSON_GetObjectItem(root, "sha");
            if (sha && cJSON_IsString(sha)) {
                out_sha = sha->valuestring;
                ret = ESP_OK;
            }
            cJSON_Delete(root);
        }
    } else if (status == 404) {
        // File doesn't exist yet, that's OK
        ret = ESP_ERR_NOT_FOUND;
    } else {
        ESP_LOGW(TAG, "Failed to get SHA: HTTP %d", status);
        ret = ESP_FAIL;
    }

    free(response);
    return ret;
}

esp_err_t GitHubBackup::loadToken() {
    // Load from secrets storage
    SecretsNVS& secrets = SecretsNVS::getInstance();
    secrets.init();

    token_ = secrets.getGitHubToken();
    return ESP_OK;
}

esp_err_t GitHubBackup::saveToken() {
    // Save to secrets storage
    SecretsNVS& secrets = SecretsNVS::getInstance();
    secrets.init();

    return secrets.setGitHubToken(token_);
}
