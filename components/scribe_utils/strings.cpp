#include "strings.h"
#include <cJSON.h>
#include <esp_log.h>
#include <cstdio>
#include <cstring>

static const char* TAG = "SCRIBE_STRINGS";

Strings& Strings::getInstance() {
    static Strings instance;
    return instance;
}

esp_err_t Strings::init(const char* primary_path, const char* fallback_path) {
    if (primary_path && loadFromFile(primary_path) == ESP_OK) {
        loaded_ = true;
        ESP_LOGI(TAG, "Loaded strings from %s", primary_path);
        return ESP_OK;
    }
    if (fallback_path && loadFromFile(fallback_path) == ESP_OK) {
        loaded_ = true;
        ESP_LOGI(TAG, "Loaded strings from %s", fallback_path);
        return ESP_OK;
    }

    loaded_ = false;
    ESP_LOGW(TAG, "Failed to load strings from any path");
    return ESP_FAIL;
}

const char* Strings::get(const char* key) const {
    if (!key) {
        return "";
    }
    auto it = strings_.find(key);
    if (it == strings_.end()) {
        return key;
    }
    return it->second.c_str();
}

std::string Strings::format(const char* key,
                            const std::initializer_list<std::pair<const char*, std::string>>& tokens) const {
    std::string result = get(key);

    for (const auto& token : tokens) {
        if (!token.first) {
            continue;
        }
        std::string marker = std::string("{") + token.first + "}";
        size_t pos = 0;
        while ((pos = result.find(marker, pos)) != std::string::npos) {
            result.replace(pos, marker.length(), token.second);
            pos += token.second.length();
        }
    }

    return result;
}

esp_err_t Strings::loadFromFile(const char* path) {
    if (!path) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE* f = fopen(path, "r");
    if (!f) {
        return ESP_ERR_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) {
        fclose(f);
        return ESP_FAIL;
    }

    std::string content;
    content.resize(static_cast<size_t>(size));
    size_t read = fread(&content[0], 1, static_cast<size_t>(size), f);
    fclose(f);

    if (read != static_cast<size_t>(size)) {
        ESP_LOGW(TAG, "Short read for %s", path);
    }

    cJSON* root = cJSON_Parse(content.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse strings JSON: %s", path);
        return ESP_FAIL;
    }

    strings_.clear();
    flattenJson(root, "");
    cJSON_Delete(root);

    return ESP_OK;
}

void Strings::flattenJson(cJSON* node, const std::string& prefix) {
    if (!node) {
        return;
    }

    if (cJSON_IsString(node)) {
        strings_[prefix] = node->valuestring ? node->valuestring : "";
        return;
    }

    if (cJSON_IsObject(node)) {
        cJSON* child = node->child;
        while (child) {
            if (child->string) {
                std::string key = prefix.empty() ? child->string : prefix + "." + child->string;
                flattenJson(child, key);
            }
            child = child->next;
        }
    }
}
