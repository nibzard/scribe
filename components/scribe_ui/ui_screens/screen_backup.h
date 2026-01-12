#pragma once

#include <lvgl.h>
#include <string>
#include <functional>
#include "github_backup.h"

// Backup settings screen - Cloud backup setup (GitHub/Gist)
class ScreenBackup {
public:
    using BackCallback = std::function<void()>;
    using ConfigureCallback = std::function<void(BackupProvider provider)>;

    ScreenBackup();
    ~ScreenBackup();

    void init();
    void show();
    void hide();

    void setBackCallback(BackCallback cb) { back_cb_ = cb; }
    void setConfigureCallback(ConfigureCallback cb) { configure_cb_ = cb; }

    // Update status from actual backup state
    void updateStatus(bool has_token, const std::string& repo_info = "");

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* description_label_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* provider_list_ = nullptr;
    lv_obj_t* back_btn_ = nullptr;

    BackCallback back_cb_;
    ConfigureCallback configure_cb_;

    bool has_token_ = false;
    std::string repo_info_;

    void createWidgets();
    void updateUI();
};

// Token input dialog for GitHub personal access token
class TokenInputDialog {
public:
    using TokenCallback = std::function<void(const std::string& token)>;
    using CancelCallback = std::function<void()>;

    static TokenInputDialog& getInstance();

    void init();
    void show(TokenCallback on_save, CancelCallback on_cancel);
    void hide();

private:
    TokenInputDialog() = default;
    ~TokenInputDialog();

    lv_obj_t* dialog_ = nullptr;
    lv_obj_t* textarea_ = nullptr;
    lv_obj_t* save_btn_ = nullptr;
    lv_obj_t* cancel_btn_ = nullptr;

    TokenCallback save_callback_;
    CancelCallback cancel_callback_;

    void createDialog();
    void validateToken();
};

// Repo configuration dialog for GitHub repository backup
class RepoConfigDialog {
public:
    using ConfigCallback = std::function<void(const std::string& owner, const std::string& repo, const std::string& branch)>;
    using CancelCallback = std::function<void()>;

    static RepoConfigDialog& getInstance();

    void init();
    void show(ConfigCallback on_save, CancelCallback on_cancel);
    void hide();

private:
    RepoConfigDialog() = default;
    ~RepoConfigDialog();

    lv_obj_t* dialog_ = nullptr;
    lv_obj_t* owner_input_ = nullptr;
    lv_obj_t* repo_input_ = nullptr;
    lv_obj_t* branch_input_ = nullptr;

    ConfigCallback save_callback_;
    CancelCallback cancel_callback_;

    void createDialog();
};
