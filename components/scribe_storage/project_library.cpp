#include "project_library.h"
#include "storage_manager.h"
#include <esp_log.h>
#include <cJSON.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <esp_system.h>

static const char* TAG = "SCRIBE_LIBRARY";

ProjectLibrary& ProjectLibrary::getInstance() {
    static ProjectLibrary instance;
    return instance;
}

esp_err_t ProjectLibrary::load() {
    filepath_ = SCRIBE_LIBRARY_JSON;

    FILE* f = fopen(filepath_.c_str(), "r");
    if (!f) {
        ESP_LOGW(TAG, "library.json not found, creating new");
        library_.version = 1;
        library_.last_open_project_id = "";
        library_.projects.clear();
        return save();
    }

    // Read file content
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::string content;
    content.resize(size);
    fread(&content[0], 1, size, f);
    fclose(f);

    // Parse JSON
    cJSON* root = cJSON_Parse(content.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse library.json");
        return ESP_FAIL;
    }

    cJSON* ver = cJSON_GetObjectItem(root, "version");
    library_.version = ver ? ver->valueint : 1;

    cJSON* last_id = cJSON_GetObjectItem(root, "last_open_project_id");
    library_.last_open_project_id = cJSON_IsString(last_id) ? last_id->valuestring : "";

    library_.projects.clear();
    cJSON* projects = cJSON_GetObjectItem(root, "projects");
    if (cJSON_IsArray(projects)) {
        cJSON* proj = nullptr;
        cJSON_ArrayForEach(proj, projects) {
            cJSON* id = cJSON_GetObjectItem(proj, "id");
            cJSON* name = cJSON_GetObjectItem(proj, "name");
            if (!cJSON_IsString(id) || !cJSON_IsString(name)) {
                continue;
            }

            ProjectInfo info;
            info.id = id->valuestring;
            info.name = name->valuestring;

            cJSON* path = cJSON_GetObjectItem(proj, "path");
            if (cJSON_IsString(path)) {
                info.path = path->valuestring;
            } else {
                info.path = std::string("Projects/") + info.id + "/";
            }
            if (!info.path.empty()) {
                std::string base = std::string(SCRIBE_BASE_PATH) + "/";
                if (info.path.rfind(base, 0) == 0) {
                    info.path = info.path.substr(base.size());
                } else if (info.path.rfind("/sdcard/", 0) == 0) {
                    size_t idx = info.path.find("Scribe/");
                    if (idx != std::string::npos) {
                        info.path = info.path.substr(idx + strlen("Scribe/"));
                    }
                }
                if (!info.path.empty() && info.path[0] == '/') {
                    info.path.erase(0, 1);
                }
                if (!info.path.empty() && info.path.back() != '/') {
                    info.path.push_back('/');
                }
            }

            cJSON* last_opened = cJSON_GetObjectItem(proj, "last_opened_utc");
            info.last_opened_utc = cJSON_IsString(last_opened) ? last_opened->valuestring : "";

            cJSON* total_words = cJSON_GetObjectItem(proj, "total_words");
            info.total_words = cJSON_IsNumber(total_words) ? total_words->valueint : 0;

            library_.projects.push_back(info);
        }
    }

    cJSON_Delete(root);
    sortProjects();
    ESP_LOGI(TAG, "Loaded library with %zu projects", library_.projects.size());
    return ESP_OK;
}

esp_err_t ProjectLibrary::save() {
    sortProjects();
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "version", library_.version);
    cJSON_AddStringToObject(root, "last_open_project_id", library_.last_open_project_id.c_str());

    cJSON* projects = cJSON_CreateArray();
    for (const auto& proj : library_.projects) {
        cJSON* p = cJSON_CreateObject();
        cJSON_AddStringToObject(p, "id", proj.id.c_str());
        cJSON_AddStringToObject(p, "name", proj.name.c_str());
        cJSON_AddStringToObject(p, "path", proj.path.c_str());
        cJSON_AddStringToObject(p, "last_opened_utc", proj.last_opened_utc.c_str());
        cJSON_AddNumberToObject(p, "total_words", proj.total_words);
        cJSON_AddItemToArray(projects, p);
    }
    cJSON_AddItemToObject(root, "projects", projects);

    char* json_str = cJSON_Print(root);

    FILE* f = fopen(filepath_.c_str(), "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open library.json for writing");
        cJSON_Delete(root);
        free(json_str);
        return ESP_FAIL;
    }

    fwrite(json_str, 1, strlen(json_str), f);
    fclose(f);

    cJSON_Delete(root);
    free(json_str);
    return ESP_OK;
}

const ProjectInfo* ProjectLibrary::getProjectById(const std::string& id) const {
    auto it = std::find_if(library_.projects.begin(), library_.projects.end(),
        [&id](const ProjectInfo& p) { return p.id == id; });
    return it != library_.projects.end() ? &(*it) : nullptr;
}

esp_err_t ProjectLibrary::updateProject(const ProjectInfo& project) {
    // Update existing or add new
    auto it = std::find_if(library_.projects.begin(), library_.projects.end(),
        [&project](const ProjectInfo& p) { return p.id == project.id; });

    if (it != library_.projects.end()) {
        *it = project;
    } else {
        library_.projects.push_back(project);
    }

    return save();
}

ProjectInfo* ProjectLibrary::findProject(const std::string& id) {
    auto it = std::find_if(library_.projects.begin(), library_.projects.end(),
        [&id](const ProjectInfo& p) { return p.id == id; });
    return it != library_.projects.end() ? &(*it) : nullptr;
}

esp_err_t ProjectLibrary::touchProjectOpened(const std::string& id, const std::string& opened_utc) {
    ProjectInfo* info = findProject(id);
    if (!info) {
        return ESP_ERR_NOT_FOUND;
    }
    info->last_opened_utc = opened_utc;
    library_.last_open_project_id = id;
    return save();
}

esp_err_t ProjectLibrary::updateProjectWordCount(const std::string& id, size_t total_words) {
    ProjectInfo* info = findProject(id);
    if (!info) {
        return ESP_ERR_NOT_FOUND;
    }
    info->total_words = total_words;
    return save();
}

esp_err_t ProjectLibrary::updateProjectSavedState(const std::string& id, size_t total_words, const std::string& last_saved_utc) {
    ProjectInfo* info = findProject(id);
    if (!info) {
        return ESP_ERR_NOT_FOUND;
    }
    info->total_words = total_words;

    std::string project_json_path = std::string(SCRIBE_PROJECTS_DIR) + "/" + id + "/project.json";
    FILE* f = fopen(project_json_path.c_str(), "r");
    if (!f) {
        ESP_LOGW(TAG, "project.json not found for %s", id.c_str());
    } else {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        std::string content;
        content.resize(size);
        fread(&content[0], 1, size, f);
        fclose(f);

        cJSON* root = cJSON_Parse(content.c_str());
        if (root) {
            cJSON* last_saved = cJSON_GetObjectItem(root, "last_saved_utc");
            if (cJSON_IsString(last_saved)) {
                cJSON_SetValuestring(last_saved, last_saved_utc.c_str());
            } else {
                cJSON_AddStringToObject(root, "last_saved_utc", last_saved_utc.c_str());
            }

            char* json_str = cJSON_Print(root);
            cJSON_Delete(root);

            if (json_str) {
                FILE* wf = fopen(project_json_path.c_str(), "w");
                if (wf) {
                    fwrite(json_str, 1, strlen(json_str), wf);
                    fclose(wf);
                }
                free(json_str);
            }
        }
    }

    return save();
}

esp_err_t ProjectLibrary::createProject(const std::string& name, std::string& out_id) {
    std::string trimmed = name;
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(),
                                               [](unsigned char c) { return !std::isspace(c); }));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(),
                               [](unsigned char c) { return !std::isspace(c); }).base(),
                  trimmed.end());
    if (trimmed.empty()) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check for duplicate names
    for (const auto& p : library_.projects) {
        if (p.name == trimmed) {
            ESP_LOGW(TAG, "Project with name '%s' already exists", name.c_str());
            return ESP_ERR_INVALID_ARG;
        }
    }

    std::string id = generateId();
    std::string path = std::string(SCRIBE_PROJECTS_DIR) + "/" + id;

    // Get current time
    time_t now = time(nullptr);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    // Create project directory
    mkdir(path.c_str(), 0755);
    mkdir((path + "/journal").c_str(), 0755);
    mkdir((path + "/snapshots").c_str(), 0755);

    // Create project.json
    cJSON* proj_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(proj_json, "version", 1);
    cJSON_AddStringToObject(proj_json, "id", id.c_str());
    cJSON_AddStringToObject(proj_json, "name", trimmed.c_str());
    cJSON_AddStringToObject(proj_json, "created_utc", time_buf);
    cJSON_AddStringToObject(proj_json, "last_saved_utc", time_buf);
    cJSON_AddNullToObject(proj_json, "last_synced_utc");

    cJSON* backup = cJSON_CreateObject();
    cJSON_AddBoolToObject(backup, "enabled", false);
    cJSON_AddNullToObject(backup, "provider");
    cJSON_AddNullToObject(backup, "repo");
    cJSON_AddNullToObject(backup, "path");
    cJSON_AddNullToObject(backup, "sha");
    cJSON_AddNumberToObject(backup, "conflict_counter", 0);
    cJSON_AddItemToObject(proj_json, "backup", backup);

    char* json_str = cJSON_Print(proj_json);

    std::string proj_file = path + "/project.json";
    FILE* f = fopen(proj_file.c_str(), "w");
    fwrite(json_str, 1, strlen(json_str), f);
    fclose(f);

    cJSON_Delete(proj_json);
    free(json_str);

    // Create empty manuscript.md
    std::string manuscript = path + "/manuscript.md";
    f = fopen(manuscript.c_str(), "w");
    fclose(f);

    // Add to library
    ProjectInfo info;
    info.id = id;
    info.name = trimmed;
    info.path = std::string("Projects/") + id + "/";
    info.last_opened_utc = time_buf;
    info.total_words = 0;

    library_.projects.push_back(info);
    library_.last_open_project_id = id;
    save();

    out_id = id;
    ESP_LOGI(TAG, "Created project '%s' with ID %s", name.c_str(), id.c_str());
    return ESP_OK;
}

std::string ProjectLibrary::generateId() {
    for (int i = 0; i < 5; i++) {
        uint32_t rand = esp_random();
        char buf[32];
        snprintf(buf, sizeof(buf), "p_%08x", rand);
        if (!findProject(buf)) {
            return buf;
        }
    }
    char fallback[32];
    snprintf(fallback, sizeof(fallback), "p_%08x", static_cast<unsigned int>(time(nullptr)));
    return fallback;
}

esp_err_t ProjectLibrary::archiveProject(const std::string& id) {
    auto it = std::find_if(library_.projects.begin(), library_.projects.end(),
        [&id](const ProjectInfo& p) { return p.id == id; });

    if (it == library_.projects.end()) {
        return ESP_ERR_NOT_FOUND;
    }

    // Move project directory to Archive
    std::string old_path = std::string(SCRIBE_BASE_PATH) + "/" + it->path;
    if (!old_path.empty() && old_path.back() == '/') {
        old_path.pop_back();
    }
    std::string new_path = std::string(SCRIBE_ARCHIVE_DIR) + "/" + id;
    mkdir(SCRIBE_ARCHIVE_DIR, 0755);
    if (rename(old_path.c_str(), new_path.c_str()) != 0) {
        ESP_LOGE(TAG, "Failed to archive project %s", id.c_str());
        return ESP_FAIL;
    }

    library_.projects.erase(it);
    return save();
}

void ProjectLibrary::sortProjects() {
    std::sort(library_.projects.begin(), library_.projects.end(),
              [](const ProjectInfo& a, const ProjectInfo& b) {
                  return a.last_opened_utc > b.last_opened_utc;
              });
}
