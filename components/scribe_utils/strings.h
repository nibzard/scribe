#pragma once

#include <esp_err.h>
#include <string>
#include <unordered_map>

struct cJSON;

// Simple string store that loads assets/strings/en.json and provides lookups.
class Strings {
public:
    static Strings& getInstance();

    // Load strings from a JSON file path. Attempts primary, then fallback.
    esp_err_t init(const char* primary_path, const char* fallback_path);

    // Get a string by key. Returns the key itself if missing.
    const char* get(const char* key) const;

    // Format a string by replacing {token} placeholders.
    std::string format(const char* key,
                       const std::initializer_list<std::pair<const char*, std::string>>& tokens) const;

    bool isLoaded() const { return loaded_; }

private:
    Strings() = default;
    ~Strings() = default;

    esp_err_t loadFromFile(const char* path);
    esp_err_t loadFromBuffer(const char* data, size_t size);
    void flattenJson(cJSON* node, const std::string& prefix);

    std::unordered_map<std::string, std::string> strings_;
    bool loaded_ = false;
};
