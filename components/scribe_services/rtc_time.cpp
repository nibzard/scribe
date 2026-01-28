#include "rtc_time.h"
#include "../scribe_hw/tab5_rx8130.h"
#include <esp_log.h>
#include <esp_sntp.h>
#include <time.h>
#include <sys/time.h>
#include <string>

static const char* TAG = "SCRIBE_TIME";

static bool time_synced = false;
static tab5::Rx8130 rtc;
static bool rtc_ready = false;

static void set_default_time() {
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

static void time_sync_notification(struct timeval *tv) {
    ESP_LOGI(TAG, "Time synchronized");
    time_synced = true;

    if (rtc_ready && tv) {
        time_t now = tv->tv_sec;
        struct tm timeinfo;
        gmtime_r(&now, &timeinfo);
        rtc.writeTime(timeinfo);
    }
}

void initTimeSync() {
    ESP_LOGI(TAG, "Initializing time sync...");

    rtc_ready = rtc.init();
    if (rtc_ready) {
        struct tm rtc_time = {};
        if (rtc.readTime(&rtc_time)) {
            time_t t = mktime(&rtc_time);
            struct timeval tv = {.tv_sec = t, .tv_usec = 0};
            settimeofday(&tv, nullptr);
        } else {
            set_default_time();
        }
    } else {
        set_default_time();
    }
}

void startSNTP() {
    ESP_LOGI(TAG, "Starting SNTP...");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification);
    esp_sntp_init();
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
