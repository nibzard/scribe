#pragma once

#include <esp_err.h>
#include <functional>
#include <string>
#include <vector>

// Backup status
enum class BackupStatus {
    IDLE,
    SYNCING,
    SUCCESS,
    FAILED,
    CONFLICT
};

// Backup provider type
enum class BackupProvider {
    GITHUB_REPO,
    GITHUB_GIST
};

// Backup status callback
using BackupStatusCallback = std::function<void(BackupStatus status, const std::string& message)>;

// GitHub/Gist backup manager (MVP3 feature)
// Based on SPECS.md section 6.8 - Cloud backup
// Offline-first: backup is opportunistic, never blocks writing
class GitHubBackup {
public:
    static GitHubBackup& getInstance();

    // Initialize backup system
    esp_err_t init();

    // Enable/disable backup
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    // Set GitHub token (stored in NVS)
    esp_err_t setToken(const std::string& token);

    // Get stored token
    std::string getToken() const;

    // Configure backup target
    esp_err_t configure(BackupProvider provider, const std::string& repo_owner,
                        const std::string& repo_name, const std::string& branch = "main");

    // Queue a backup request for a project
    esp_err_t queueBackup(const std::string& project_id, const std::string& content);

    // Sync now (manual trigger)
    esp_err_t syncNow();

    // Get current status
    BackupStatus getStatus() const { return status_; }

    // Register status callback
    void setStatusCallback(BackupStatusCallback callback) { status_callback_ = callback; }

    // Check if online
    bool isOnline() const { return online_; }

    // Set online status (called by WiFi manager)
    void setOnline(bool online);

private:
    GitHubBackup() = default;
    ~GitHubBackup() = default;

    bool enabled_ = false;
    bool online_ = false;
    BackupStatus status_ = BackupStatus::IDLE;
    BackupProvider provider_ = BackupProvider::GITHUB_REPO;

    std::string token_;
    std::string repo_owner_;
    std::string repo_name_;
    std::string branch_;

    BackupStatusCallback status_callback_;

    // Pending backup queue
    struct BackupRequest {
        std::string project_id;
        std::string content;
        std::string last_sha;  // For conflict detection
    };
    std::vector<BackupRequest> queue_;

    // HTTP client methods
    esp_err_t uploadToGitHub(const std::string& path, const std::string& content,
                             const std::string& message);

    esp_err_t uploadToGist(const std::string& description, const std::string& content);

    esp_err_t checkForConflict(const std::string& path, std::string& out_sha);

    esp_err_t getCommitSHA(const std::string& path, std::string& out_sha);

    // Process queue when online
    void processQueue();

    // Format API path for project
    std::string getProjectPath(const std::string& project_id) const;

    // Read token from NVS
    esp_err_t loadToken();

    // Save token to NVS
    esp_err_t saveToken();
};
