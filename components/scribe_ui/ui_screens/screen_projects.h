#pragma once

#include <lvgl.h>
#include <string>
#include <vector>
#include <functional>

// Project info for list display
struct ProjectListItem {
    std::string id;
    std::string name;
    std::string last_opened;
    size_t word_count;
};

// Project switcher screen
class ScreenProjects {
public:
    ScreenProjects();
    ~ScreenProjects();

    void init();
    void show();
    void hide();

    // Set project list
    void setProjects(const std::vector<ProjectListItem>& projects);
    void setQuery(const std::string& query);
    const std::string& getQuery() const { return query_; }
    void appendQueryChar(char c);
    void backspaceQuery();
    std::string getSelectedProjectId() const;
    std::string getSelectedProjectName() const;

    // Navigation
    void moveSelection(int delta);
    void selectCurrent();
    void filter(const std::string& query);

    // Archive current project
    void archiveCurrent();
    void confirmArchive();
    void cancelArchive();

    // Check if archive dialog is showing
    bool isArchiveDialogShowing() const { return archive_dialog_ != nullptr; }

    // Callbacks
    void setOpenCallback(std::function<void(const std::string& id)> cb) { open_callback_ = cb; }
    void setNewCallback(std::function<void()> cb) { new_callback_ = cb; }
    void setCloseCallback(std::function<void()> cb) { close_callback_ = cb; }
    void setArchiveCallback(std::function<void(const std::string& id)> cb) { archive_callback_ = cb; }

private:
    lv_obj_t* screen_;
    lv_obj_t* list_;
    lv_obj_t* search_bar_;
    lv_obj_t* archive_dialog_{nullptr};
    int selected_index_ = 0;
    std::string query_;
    std::vector<ProjectListItem> projects_;
    std::vector<size_t> filtered_indices_;
    std::vector<lv_obj_t*> buttons_;

    std::function<void(const std::string& id)> open_callback_;
    std::function<void()> new_callback_;
    std::function<void()> close_callback_;
    std::function<void(const std::string& id)> archive_callback_;

    void createWidgets();
    void updateSelection();
    void showArchiveDialog(const std::string& project_name);
    void hideArchiveDialog();
};
