#include <esp_log.h>
#include <esp_sntp.h>
#include <time.h>
#include <sys/time.h>
#include <string>

static const char* TAG = "SCRIBE_TIME";

static bool time_synced = false;

void time_sync_notification(struct timeval *tv) {
    ESP_LOGI(TAG, "Time synchronized");
    time_synced = true;
}

void initTimeSync() {
    ESP_LOGI(TAG, "Initializing time sync...");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification);
    sntp_init();

    // Set initial time to a reasonable default
    struct tm ti = {};
    ti.tm_year = 2026 - 1900;
    ti.tm_mon = 0;
    ti.tm_mday = 12;
    ti.tm_hour = 0;
    ti.tm_min = 0;
    ti.tm_sec = 0;
    time_t t = mktime(&ti);
    struct timeval tv = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&tv, nullptr);
}

bool isTimeSynced() {
    return time_synced;
}

std::string getCurrentTimeISO() {
    time_t now;
    time(&now);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    return std::string(buf);
}
