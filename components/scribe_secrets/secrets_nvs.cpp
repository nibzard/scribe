#include "secrets_nvs.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>

static const char* TAG = "SCRIBE_SECRETS";
static const char* NVS_NAMESPACE = "scribe_secrets";

SecretsNVS& SecretsNVS::getInstance() {
    static SecretsNVS instance;
    return instance;
}

esp_err_t SecretsNVS::init() {
    if (initialized_) return ESP_OK;

    ESP_LOGI(TAG, "Initializing secrets storage...");

    // Open namespace
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_close(handle);
        initialized_ = true;
    }

    return err;
}

esp_err_t SecretsNVS::setGitHubToken(const std::string& token) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_str(handle, "github_token", token.c_str());
    if (err == ESP_OK) {
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "GitHub token saved (length: %zu)", token.length());
    } else {
        ESP_LOGE(TAG, "Failed to save GitHub token");
    }

    nvs_close(handle);
    return err;
}

std::string SecretsNVS::getGitHubToken() const {
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return "";
    }

    size_t len = 0;
    nvs_get_str(handle, "github_token", nullptr, &len);
    if (len == 0) {
        nvs_close(handle);
        return "";
    }

    std::string token;
    token.resize(len);
    nvs_get_str(handle, "github_token", &token[0], &len);
    token.resize(len - 1);  // Remove null terminator

    nvs_close(handle);
    return token;
}

void SecretsNVS::clearGitHubToken() {
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return;

    nvs_erase_key(handle, "github_token");
    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "GitHub token cleared");
}

esp_err_t SecretsNVS::setOpenAIKey(const std::string& key) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_str(handle, "openai_key", key.c_str());
    if (err == ESP_OK) {
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "OpenAI key saved (length: %zu)", key.length());
    }

    nvs_close(handle);
    return err;
}

std::string SecretsNVS::getOpenAIKey() const {
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return "";
    }

    size_t len = 0;
    nvs_get_str(handle, "openai_key", nullptr, &len);
    if (len == 0) {
        nvs_close(handle);
        return "";
    }

    std::string key;
    key.resize(len);
    nvs_get_str(handle, "openai_key", &key[0], &len);
    key.resize(len - 1);

    nvs_close(handle);
    return key;
}

void SecretsNVS::clearOpenAIKey() {
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return;

    nvs_erase_key(handle, "openai_key");
    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "OpenAI key cleared");
}

esp_err_t SecretsNVS::wipeAll() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    nvs_erase_all(handle);
    err = nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "All secrets wiped");
    return err;
}
