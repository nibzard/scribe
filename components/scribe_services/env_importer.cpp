#include "env_importer.h"
#include "ai_assist.h"
#include "github_backup.h"
#include <esp_log.h>
#include <cstdio>
#include <cerrno>
#include <cctype>
#include <cstring>

static const char* TAG = "SCRIBE_ENV_IMPORT";

namespace {
std::string trim(const std::string& input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        start++;
    }
    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        end--;
    }
    return input.substr(start, end - start);
}

void stripQuotes(std::string& value) {
    if (value.size() < 2) {
        return;
    }
    char first = value.front();
    char last = value.back();
    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
        value = value.substr(1, value.size() - 2);
    }
}
} // namespace

EnvImportResult importEnvFile(const char* path) {
    EnvImportResult result;
    if (!path || !path[0]) {
        result.error = "Invalid path";
        return result;
    }

    FILE* f = fopen(path, "r");
    if (!f) {
        if (errno == ENOENT) {
            return result;
        }
        char buf[128];
        snprintf(buf, sizeof(buf), "Failed to open .env: %s", strerror(errno));
        result.error = buf;
        return result;
    }

    result.found = true;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        std::string raw(line);
        std::string trimmed = trim(raw);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        if (trimmed.rfind("export ", 0) == 0) {
            trimmed = trim(trimmed.substr(7));
        }

        size_t eq = trimmed.find('=');
        if (eq == std::string::npos) {
            continue;
        }

        std::string key = trim(trimmed.substr(0, eq));
        std::string value = trim(trimmed.substr(eq + 1));
        if (key.empty()) {
            continue;
        }
        stripQuotes(value);
        if (value.empty()) {
            continue;
        }

        if (key == "OPENAI_API_KEY") {
            result.keys_found++;
            esp_err_t ret = AIAssist::getInstance().setAPIKey(value);
            if (ret == ESP_OK) {
                result.ai_key_set = true;
            } else if (result.error.empty()) {
                result.error = "OpenAI key invalid";
            }
        } else if (key == "GITHUB_TOKEN" || key == "GITHUB_API_TOKEN") {
            result.keys_found++;
            esp_err_t ret = GitHubBackup::getInstance().setToken(value);
            if (ret == ESP_OK) {
                result.github_token_set = true;
            } else if (result.error.empty()) {
                result.error = "GitHub token invalid";
            }
        }
    }

    fclose(f);

    if (result.keys_found == 0) {
        ESP_LOGI(TAG, "No recognized keys in .env");
    }

    return result;
}
