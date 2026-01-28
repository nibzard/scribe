#include "settings_store.h"
#include "storage_manager.h"
#include <cJSON.h>
#include <esp_log.h>
#include <cstring>
#include <errno.h>

static const char* TAG = "SCRIBE_SETTINGS_STORE";
namespace {
constexpr int kFontSizeMin = 12;
constexpr int kFontSizeMax = 28;
constexpr int kLegacyFontSizes[] = {14, 16, 20};
}  // namespace

SettingsStore& SettingsStore::getInstance() {
    static SettingsStore instance;
    return instance;
}

esp_err_t SettingsStore::init() {
    ESP_LOGI(TAG, "Initializing settings store");
    return ESP_OK;
}

esp_err_t SettingsStore::load(AppSettings& settings) {
    FILE* f = fopen(SCRIBE_SETTINGS_JSON, "r");
    if (!f) {
        ESP_LOGW(TAG, "settings.json not found, creating defaults");
        settings = AppSettings();
        save(settings);
        return ESP_ERR_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::string content;
    content.resize(size);
    fread(&content[0], 1, size, f);
    fclose(f);

    cJSON* root = cJSON_Parse(content.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse settings.json, using defaults");
        settings = AppSettings();
        return ESP_FAIL;
    }

    settings = AppSettings();

    int settings_version = 1;
    cJSON* version = cJSON_GetObjectItem(root, "version");
    if (cJSON_IsNumber(version)) {
        settings_version = version->valueint;
    }

    cJSON* theme_id = cJSON_GetObjectItem(root, "theme_id");
    if (cJSON_IsString(theme_id) && theme_id->valuestring) {
        settings.theme_id = theme_id->valuestring;
    } else {
        cJSON* dark = cJSON_GetObjectItem(root, "dark_theme");
        if (cJSON_IsBool(dark)) {
            settings.theme_id = cJSON_IsTrue(dark) ? "scribe_dark" : "scribe_light";
        }
    }
    if (settings.theme_id.empty()) {
        settings.theme_id = "dracula";
    }

    cJSON* font = cJSON_GetObjectItem(root, "font_size");
    if (cJSON_IsNumber(font)) {
        settings.font_size = font->valueint;
    }
    if (settings_version < 4) {
        if (settings.font_size >= 0 &&
            settings.font_size < static_cast<int>(sizeof(kLegacyFontSizes) / sizeof(kLegacyFontSizes[0]))) {
            settings.font_size = kLegacyFontSizes[settings.font_size];
        }
    }
    if (settings.font_size < kFontSizeMin) {
        settings.font_size = kFontSizeMin;
    } else if (settings.font_size > kFontSizeMax) {
        settings.font_size = kFontSizeMax;
    }

    cJSON* layout = cJSON_GetObjectItem(root, "keyboard_layout");
    if (cJSON_IsNumber(layout)) {
        settings.keyboard_layout = layout->valueint;
    }

    cJSON* sleep = cJSON_GetObjectItem(root, "auto_sleep");
    if (cJSON_IsNumber(sleep)) {
        settings.auto_sleep = sleep->valueint;
    }

    cJSON* orientation = cJSON_GetObjectItem(root, "display_orientation");
    if (cJSON_IsNumber(orientation)) {
        int value = orientation->valueint;
        if (settings_version < 2) {
            if (value == 0) {
                settings.display_orientation = 1;
            } else if (value == 1) {
                settings.display_orientation = 2;
            } else {
                settings.display_orientation = 0;
            }
        } else {
            settings.display_orientation = value;
        }
    }
    if (settings.display_orientation < 0 || settings.display_orientation > 4) {
        settings.display_orientation = 0;
    }

    cJSON* wifi = cJSON_GetObjectItem(root, "wifi_enabled");
    if (cJSON_IsBool(wifi)) {
        settings.wifi_enabled = cJSON_IsTrue(wifi);
    }

    cJSON* backup = cJSON_GetObjectItem(root, "backup_enabled");
    if (cJSON_IsBool(backup)) {
        settings.backup_enabled = cJSON_IsTrue(backup);
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Settings loaded: theme=%s, font=%d, sleep=%d",
             settings.theme_id.c_str(),
             settings.font_size,
             settings.auto_sleep);

    return ESP_OK;
}

esp_err_t SettingsStore::save(const AppSettings& settings) {
    StorageManager::getInstance().ensureDirectories();

    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "version", 4);
    const char* theme_id = settings.theme_id.empty() ? "dracula" : settings.theme_id.c_str();
    cJSON_AddStringToObject(root, "theme_id", theme_id);
    cJSON_AddNumberToObject(root, "font_size", settings.font_size);
    cJSON_AddNumberToObject(root, "keyboard_layout", settings.keyboard_layout);
    cJSON_AddNumberToObject(root, "auto_sleep", settings.auto_sleep);
    cJSON_AddNumberToObject(root, "display_orientation", settings.display_orientation);
    cJSON_AddBoolToObject(root, "wifi_enabled", settings.wifi_enabled);
    cJSON_AddBoolToObject(root, "backup_enabled", settings.backup_enabled);

    char* json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_str) {
        return ESP_FAIL;
    }

    FILE* f = fopen(SCRIBE_SETTINGS_JSON, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open settings.json for writing: %s", strerror(errno));
        free(json_str);
        return ESP_FAIL;
    }

    fwrite(json_str, 1, strlen(json_str), f);
    fclose(f);
    free(json_str);

    ESP_LOGI(TAG, "Settings saved");
    return ESP_OK;
}
