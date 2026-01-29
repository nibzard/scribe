#include "screen_projects.h"
#include "../../scribe_utils/strings.h"
#include "../theme/theme.h"
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
    if (archive_dialog_) {
        lv_obj_delete(archive_dialog_);
    }
    if (screen_) {
        lv_obj_delete(screen_);
    }
}

void ScreenProjects::init() {
    ESP_LOGI(TAG, "Initializing projects screen");

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_HOR_RES, LV_VER_RES);
    Theme::applyScreenStyle(screen_);

    createWidgets();
}

void ScreenProjects::createWidgets() {
    // Title
    title_ = lv_label_create(screen_);
    lv_label_set_text(title_, Strings::getInstance().get("switch_project.title"));
    lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(20));
    lv_obj_set_style_text_font(title_, Theme::getUIFont(Theme::UiFontRole::Title), 0);

    // Search bar
    search_bar_ = lv_textarea_create(screen_);
    lv_obj_set_size(search_bar_, Theme::fitWidth(300, 40), Theme::scalePx(40));
    lv_obj_align(search_bar_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(60));
    lv_textarea_set_placeholder_text(search_bar_, Strings::getInstance().get("switch_project.search_hint"));
    lv_textarea_set_one_line(search_bar_, true);
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_color(search_bar_, colors.fg, 0);
    lv_obj_set_style_bg_opa(search_bar_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(search_bar_, colors.border, 0);
    lv_obj_set_style_border_width(search_bar_, 1, 0);
    lv_obj_set_style_text_color(search_bar_, colors.text, 0);
    lv_obj_set_style_text_color(search_bar_, colors.text_secondary, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_font(search_bar_, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Project list
    list_ = lv_list_create(screen_);
    lv_obj_set_size(list_, Theme::fitWidth(350, 40), Theme::fitHeight(350, 200));
    lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(120));
    lv_obj_set_style_bg_color(list_, colors.fg, 0);
    lv_obj_set_style_bg_opa(list_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(list_, colors.border, 0);
    lv_obj_set_style_border_width(list_, 1, 0);
    lv_obj_set_style_text_font(list_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
}

void ScreenProjects::show() {
    if (screen_) {
        Theme::applyScreenStyle(screen_);
        const Theme::Colors& colors = Theme::getColors();
        lv_obj_set_style_text_font(screen_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        if (title_) {
            lv_obj_set_style_text_color(title_, colors.text, 0);
            lv_obj_set_style_text_font(title_, Theme::getUIFont(Theme::UiFontRole::Title), 0);
            lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(20));
        }
        if (search_bar_) {
            lv_obj_set_size(search_bar_, Theme::fitWidth(300, 40), Theme::scalePx(40));
            lv_obj_align(search_bar_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(60));
            lv_obj_set_style_bg_color(search_bar_, colors.fg, 0);
            lv_obj_set_style_bg_opa(search_bar_, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(search_bar_, colors.border, 0);
            lv_obj_set_style_border_width(search_bar_, 1, 0);
            lv_obj_set_style_text_color(search_bar_, colors.text, 0);
            lv_obj_set_style_text_color(search_bar_, colors.text_secondary, LV_PART_TEXTAREA_PLACEHOLDER);
            lv_obj_set_style_text_font(search_bar_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        if (list_) {
            lv_obj_set_size(list_, Theme::fitWidth(350, 40), Theme::fitHeight(350, 200));
            lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, Theme::scalePx(120));
            lv_obj_set_style_bg_color(list_, colors.fg, 0);
            lv_obj_set_style_bg_opa(list_, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(list_, colors.border, 0);
            lv_obj_set_style_border_width(list_, 1, 0);
            lv_obj_set_style_text_font(list_, Theme::getUIFont(Theme::UiFontRole::Body), 0);
        }
        for (auto* btn : buttons_) {
            if (!btn) continue;
            lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
        }
        lv_screen_load(screen_);
        if (search_bar_) {
            lv_obj_add_state(search_bar_, LV_STATE_FOCUSED);
        }
    }
}

void ScreenProjects::hide() {
    hideArchiveDialog();
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
        lv_list_add_button(list_, LV_SYMBOL_LIST, Strings::getInstance().get("switch_project.empty"));
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
        lv_list_add_button(list_, LV_SYMBOL_LIST, Strings::getInstance().get("switch_project.no_results"));
        selected_index_ = 0;
        return;
    }

    const Theme::Colors& colors = Theme::getColors();
    for (size_t i = 0; i < filtered_indices_.size(); i++) {
        const auto& proj = projects_[filtered_indices_[i]];
        char buf[256];
        snprintf(buf, sizeof(buf), "%s (%zu words)", proj.name.c_str(), proj.word_count);
        lv_obj_t* btn = lv_list_add_button(list_, LV_SYMBOL_FILE, buf);
        lv_obj_set_style_bg_color(btn, colors.selection, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, colors.text, LV_PART_MAIN);
        buttons_.push_back(btn);
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

std::string ScreenProjects::getSelectedProjectName() const {
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(filtered_indices_.size())) {
        return "";
    }
    size_t index = filtered_indices_[selected_index_];
    if (index >= projects_.size()) {
        return "";
    }
    return projects_[index].name;
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

void ScreenProjects::archiveCurrent() {
    std::string project_name = getSelectedProjectName();
    if (!project_name.empty()) {
        showArchiveDialog(project_name);
    }
}

void ScreenProjects::confirmArchive() {
    std::string id = getSelectedProjectId();
    hideArchiveDialog();

    if (!id.empty() && archive_callback_) {
        archive_callback_(id);
    }

    // Refresh the list after archiving
    filter(query_);
}

void ScreenProjects::cancelArchive() {
    hideArchiveDialog();
}

void ScreenProjects::showArchiveDialog(const std::string& project_name) {
    if (archive_dialog_) {
        return;  // Already showing
    }

    // Create modal dialog
    archive_dialog_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(archive_dialog_, LV_HOR_RES - Theme::scalePx(40), LV_VER_RES - Theme::scalePx(80));
    lv_obj_align(archive_dialog_, LV_ALIGN_CENTER, 0, 0);
    const Theme::Colors& colors = Theme::getColors();
    lv_obj_set_style_bg_opa(archive_dialog_, LV_OPA_80, 0);
    lv_obj_set_style_bg_color(archive_dialog_, colors.shadow, 0);

    // Create dialog content
    lv_obj_t* content = lv_obj_create(archive_dialog_);
    lv_obj_set_size(content, LV_HOR_RES - Theme::scalePx(80), Theme::scalePx(120));
    lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(content, colors.fg, 0);
    lv_obj_set_style_border_color(content, colors.border, 0);
    lv_obj_set_style_border_width(content, 1, 0);
    lv_obj_set_style_pad_all(content, Theme::scalePx(20), 0);
    lv_obj_set_style_text_font(content, Theme::getUIFont(Theme::UiFontRole::Body), 0);

    // Title
    lv_obj_t* title = lv_label_create(content);
    lv_label_set_text(title, Strings::getInstance().get("switch_project.archive_title"));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, Theme::scalePx(10));
    lv_obj_set_style_text_color(title, colors.text, 0);
    lv_obj_set_style_text_font(title, Theme::getUIFont(Theme::UiFontRole::Title), 0);

    // Message
    lv_obj_t* msg = lv_label_create(content);
    Strings& strings = Strings::getInstance();
    std::string msg_text = strings.format("switch_project.archive_body",
        {{"name", project_name}});
    lv_label_set_text(msg, msg_text.c_str());
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(msg, colors.text, 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);

    // Confirm button hint
    lv_obj_t* hint = lv_label_create(content);
    lv_label_set_text(hint, Strings::getInstance().get("switch_project.archive_hint"));
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -Theme::scalePx(10));
    lv_obj_set_style_text_color(hint, colors.text_secondary, 0);
    lv_obj_set_style_text_font(hint, Theme::getUIFont(Theme::UiFontRole::Small), 0);
}

void ScreenProjects::hideArchiveDialog() {
    if (archive_dialog_) {
        lv_obj_delete(archive_dialog_);
        archive_dialog_ = nullptr;
    }
}
