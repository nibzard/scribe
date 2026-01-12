#include "wifi_manager.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <cstring>

static const char* TAG = "SCRIBE_WIFI";

// WiFi event group bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Maximum number of networks to store from scan
#define MAX_SCAN_NETWORKS 20

// Scan result storage
static wifi_ap_record_t* s_scan_records = nullptr;
static int s_scan_count = 0;

// Forward declarations for event handlers
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);

WiFiManager& WiFiManager::getInstance() {
    static WiFiManager instance;
    return instance;
}

esp_err_t WiFiManager::init() {
    ESP_LOGI(TAG, "Initializing WiFi manager (opportunistic mode)...");

    esp_err_t ret = initWiFiStack();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi init failed: %s - WiFi will be unavailable", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "WiFi manager initialized");
    return ESP_OK;
}

esp_err_t WiFiManager::initWiFiStack() {
    // Initialize TCP/IP stack
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create default WiFi station interface
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init WiFi: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set WiFi mode to station
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register event handlers
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, this);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        return ret;
    }

    // Allocate scan result storage
    s_scan_records = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * MAX_SCAN_NETWORKS);
    if (!s_scan_records) {
        ESP_LOGE(TAG, "Failed to allocate scan storage");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t WiFiManager::setEnabled(bool enabled) {
    bool was_enabled = enabled_.load();
    enabled_.store(enabled);

    if (enabled && !was_enabled) {
        ESP_LOGI(TAG, "Enabling WiFi...");
        return startWiFi();
    } else if (!enabled && was_enabled) {
        ESP_LOGI(TAG, "Disabling WiFi...");
        return stopWiFi();
    }

    return ESP_OK;
}

esp_err_t WiFiManager::startWiFi() {
    esp_err_t ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        enabled_.store(false);
        return ret;
    }

    ESP_LOGI(TAG, "WiFi started");
    return ESP_OK;
}

esp_err_t WiFiManager::stopWiFi() {
    // Disconnect first if connected
    if (connected_.load()) {
        disconnect();
    }

    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to stop WiFi: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "WiFi stopped");
    return ESP_OK;
}

std::string WiFiManager::getSSID() const {
    return current_ssid_;
}

esp_err_t WiFiManager::startScan(WiFiScanCallback callback) {
    if (!enabled_.load()) {
        ESP_LOGE(TAG, "WiFi is not enabled");
        return ESP_ERR_INVALID_STATE;
    }

    if (connected_.load()) {
        // Must disconnect to scan
        disconnect();
    }

    scan_callback_ = callback;

    ESP_LOGI(TAG, "Starting WiFi scan...");
    esp_err_t ret = esp_wifi_scan_start(NULL, false);  // Don't block
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start scan: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

esp_err_t WiFiManager::connect(const std::string& ssid, const std::string& password) {
    if (!enabled_.load()) {
        ESP_LOGE(TAG, "WiFi is not enabled");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Connecting to '%s'...", ssid.c_str());

    // Configure WiFi credentials
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password) - 1);

    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start connection
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect: %s", esp_err_to_name(ret));
        return ret;
    }

    current_ssid_ = ssid;
    return ESP_OK;
}

esp_err_t WiFiManager::disconnect() {
    ESP_LOGI(TAG, "Disconnecting WiFi...");
    esp_err_t ret = esp_wifi_disconnect();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Disconnect failed: %s", esp_err_to_name(ret));
    }

    connected_.store(false);
    current_ssid_.clear();

    if (status_callback_) {
        status_callback_(false, "");
    }

    return ESP_OK;
}

std::string WiFiManager::getMacAddress() const {
    uint8_t mac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if (ret != ESP_OK) {
        return "";
    }

    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(mac_str);
}

std::string WiFiManager::getIPAddress() const {
    if (!connected_.load()) {
        return "";
    }

    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) {
        return "";
    }

    esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
    if (ret != ESP_OK) {
        return "";
    }

    // Convert IP to string
    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
    return std::string(ip_str);
}

void WiFiManager::handleWiFiEvent(int32_t event_id, void* event_data) {
    switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGD(TAG, "WiFi station started");
            break;

        case WIFI_EVENT_STA_CONNECTED: {
            wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*)event_data;
            ESP_LOGI(TAG, "Connected to %s", event->ssid);
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*)event_data;
            ESP_LOGW(TAG, "Disconnected from %s, reason: %d", event->ssid, event->reason);

            bool was_connected = connected_.exchange(false);
            current_ssid_.clear();

            if (was_connected && status_callback_) {
                status_callback_(false, "");
            }

            // Don't auto-reconnect in opportunistic mode
            // User will manually reconnect if needed
            break;
        }

        case WIFI_EVENT_SCAN_DONE: {
            wifi_event_sta_scan_done_t* event = (wifi_event_sta_scan_done_t*)event_data;
            ESP_LOGI(TAG, "Scan done: status=%d, count=%d", event->status, event->number);

            if (event->status == 0) {
                s_scan_count = event->number;
                if (s_scan_count > MAX_SCAN_NETWORKS) {
                    s_scan_count = MAX_SCAN_NETWORKS;
                }

                esp_wifi_scan_get_ap_records(&s_scan_count, s_scan_records);

                // Convert to callback format
                std::vector<WiFiNetwork> networks;
                for (int i = 0; i < s_scan_count; i++) {
                    WiFiNetwork net;
                    net.ssid = (char*)s_scan_records[i].ssid;
                    net.rssi = s_scan_records[i].rssi;
                    net.secure = (s_scan_records[i].authmode != WIFI_AUTH_OPEN);
                    networks.push_back(net);
                }

                if (scan_callback_) {
                    scan_callback_(networks);
                    scan_callback_ = nullptr;  // One-shot callback
                }
            } else {
                ESP_LOGW(TAG, "Scan failed: %d", event->status);
                if (scan_callback_) {
                    scan_callback_({});
                    scan_callback_ = nullptr;
                }
            }
            break;
        }

        default:
            break;
    }
}

void WiFiManager::handleIPEvent(int32_t event_id, void* event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        connected_.store(true);

        if (status_callback_) {
            status_callback_(true, current_ssid_);
        }
    }
}

// C wrapper for event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    WiFiManager* mgr = static_cast<WiFiManager*>(arg);

    if (event_base == WIFI_EVENT) {
        mgr->handleWiFiEvent(event_id, event_data);
    } else if (event_base == IP_EVENT) {
        mgr->handleIPEvent(event_id, event_data);
    }
}
