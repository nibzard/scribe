#pragma once

#include <esp_err.h>
#include <string>
#include <vector>

// Project metadata
struct ProjectInfo {
    std::string id;
    std::string name;
    std::string path;
    std::string last_opened_utc;
    size_t total_words;
};

// Library JSON structure
struct Library {
    int version;
    std::string last_open_project_id;
    std::vector<ProjectInfo> projects;
};

// Manages library.json and project list
class ProjectLibrary {
public:
    static ProjectLibrary& getInstance();

    // Load library.json from SD
    esp_err_t load();

    // Save library.json to SD
    esp_err_t save();

    // Get all projects (sorted by last opened)
    const std::vector<ProjectInfo>& getProjects() const { return library_.projects; }

    // Get last opened project ID
    const std::string& getLastOpenedProjectId() const { return library_.last_open_project_id; }

    // Get project info by ID
    const ProjectInfo* getProjectById(const std::string& id) const;

    // Add or update project in library
    esp_err_t updateProject(const ProjectInfo& project);

    // Set last opened project
    void setLastOpenedProject(const std::string& id) { library_.last_open_project_id = id; }

    // Update last-opened time for a project
    esp_err_t touchProjectOpened(const std::string& id, const std::string& opened_utc);

    // Update word count for a project
    esp_err_t updateProjectWordCount(const std::string& id, size_t total_words);

    // Update last-saved timestamp and word count for a project
    esp_err_t updateProjectSavedState(const std::string& id, size_t total_words, const std::string& last_saved_utc);

    // Create new project
    esp_err_t createProject(const std::string& name, std::string& out_id);

    // Archive project (move to Archive directory)
    esp_err_t archiveProject(const std::string& id);

private:
    ProjectLibrary() = default;
    ~ProjectLibrary() = default;

    Library library_;
    std::string filepath_;

    // Generate unique project ID
    std::string generateId();

    // Sort projects by last opened (descending)
    void sortProjects();

    ProjectInfo* findProject(const std::string& id);
};
