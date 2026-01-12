#include "screen_projects.h"
#include <esp_log.h>
#include <algorithm>
#include <cctype>
#include <cstdio>

static const char* TAG = "SCRIBE_SCREEN_PROJECTS";

static std::string toLower(const std::string& input) {
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

ScreenProjects::ScreenProjects() : screen_(nullptr), list_(nullptr), search_bar_(nullptr), selected_index_(0) {
}

ScreenProjects::~ScreenProjects() {
    if (screen_) {
        lv_obj_del(screen_);
    }
}

void ScreenProjects::init() {
    ESP_LOGI(TAG, "Initializing projects screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);

    createWidgets();
}

void ScreenProjects::createWidgets() {
    // Title
    lv_obj_t* title = lv_label_create(screen_);
    lv_label_set_text(title, "Switch project");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Search bar
    search_bar_ = lv_textarea_create(screen_);
    lv_obj_set_size(search_bar_, 300, 40);
    lv_obj_align(search_bar_, LV_ALIGN_TOP_MID, 0, 60);
    lv_textarea_set_placeholder_text(search_bar_, "Type to search\u2026");
    lv_textarea_set_one_line(search_bar_, true);

    // Project list
    list_ = lv_list_create(screen_);
    lv_obj_set_size(list_, 350, 350);
    lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, 120);
}

void ScreenProjects::show() {
    if (screen_) {
        lv_scr_load(screen_);
        if (search_bar_) {
            lv_obj_add_state(search_bar_, LV_STATE_FOCUSED);
        }
    }
}

void ScreenProjects::hide() {
    if (close_callback_) {
        close_callback_();
    }
}

void ScreenProjects::setProjects(const std::vector<ProjectListItem>& projects) {
    projects_ = projects;
    filter(query_);
}

void ScreenProjects::setQuery(const std::string& query) {
    query_ = query;
    if (search_bar_) {
        lv_textarea_set_text(search_bar_, query_.c_str());
    }
    filter(query_);
}

void ScreenProjects::appendQueryChar(char c) {
    query_.push_back(c);
    setQuery(query_);
}

void ScreenProjects::backspaceQuery() {
    if (!query_.empty()) {
        query_.pop_back();
        setQuery(query_);
    }
}

void ScreenProjects::filter(const std::string& query) {
    if (!list_) return;

    lv_obj_clean(list_);
    buttons_.clear();
    filtered_indices_.clear();

    if (projects_.empty()) {
        lv_list_add_btn(list_, LV_SYMBOL_INFO, "No projects yet. Press Ctrl+N to create one.");
        selected_index_ = 0;
        return;
    }

    std::string q = toLower(query);
    for (size_t i = 0; i < projects_.size(); i++) {
        const auto& proj = projects_[i];
        if (q.empty() || toLower(proj.name).find(q) != std::string::npos) {
            filtered_indices_.push_back(i);
        }
    }

    if (filtered_indices_.empty()) {
        lv_list_add_btn(list_, LV_SYMBOL_INFO, "No matches.");
        selected_index_ = 0;
        return;
    }

    for (size_t i = 0; i < filtered_indices_.size(); i++) {
        const auto& proj = projects_[filtered_indices_[i]];
        char buf[256];
        snprintf(buf, sizeof(buf), "%s (%zu words)", proj.name.c_str(), proj.word_count);
        buttons_.push_back(lv_list_add_btn(list_, LV_SYMBOL_FILE, buf));
    }

    selected_index_ = 0;
    updateSelection();
}

void ScreenProjects::moveSelection(int delta) {
    int count = static_cast<int>(buttons_.size());
    if (count <= 0) return;

    selected_index_ += delta;

    if (selected_index_ < 0) selected_index_ = count - 1;
    if (selected_index_ >= count) selected_index_ = 0;

    updateSelection();
}

std::string ScreenProjects::getSelectedProjectId() const {
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(filtered_indices_.size())) {
        return "";
    }
    size_t index = filtered_indices_[selected_index_];
    if (index >= projects_.size()) {
        return "";
    }
    return projects_[index].id;
}

void ScreenProjects::selectCurrent() {
    std::string id = getSelectedProjectId();
    if (!id.empty() && open_callback_) {
        open_callback_(id);
    }
}

void ScreenProjects::updateSelection() {
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (static_cast<int>(i) == selected_index_) {
            lv_obj_add_state(buttons_[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(buttons_[i], LV_STATE_CHECKED);
        }
    }
    lv_obj_invalidate(list_);
}
