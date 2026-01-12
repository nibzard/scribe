#pragma once

#include <esp_err.h>
#include <string>

// NVS-based secrets storage (tokens, API keys)
class SecretsNVS {
public:
    static SecretsNVS& getInstance();

    esp_err_t init();

    // Store GitHub token for backup
    esp_err_t setGitHubToken(const std::string& token);
    std::string getGitHubToken() const;
    void clearGitHubToken();

    // Store OpenAI API key for AI features
    esp_err_t setOpenAIKey(const std::string& key);
    std::string getOpenAIKey() const;
    void clearOpenAIKey();

    // Wipe all secrets
    esp_err_t wipeAll();

private:
    SecretsNVS() = default;
    ~SecretsNVS() = default;

    bool initialized_ = false;
};
