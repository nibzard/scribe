/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal/hal_esp32.h"
#include <mooncake_log.h>
#include <vector>
#include <memory>
#include <string.h>
#include <bsp/m5stack_tab5.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_http_server.h>

#define TAG "wifi"

#define WIFI_SSID    "M5Tab5-UserDemo-WiFi"
#define WIFI_PASS    ""
#define MAX_STA_CONN 4

// HTTP 处理函数
esp_err_t hello_get_handler(httpd_req_t* req)
{
    const char* html_response = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Hello</title>
            <style>
                body {
                    display: flex;
                    flex-direction: column;
                    justify-content: center;
                    align-items: center;
                    height: 100vh;
                    margin: 0;
                    font-family: sans-serif;
                    background-color: #f0f0f0;
                }
                h1 {
                    font-size: 48px;
                    color: #333;
                    margin: 0;
                }
                p {
                    font-size: 18px;
                    color: #666;
                    margin-top: 10px;
                }
            </style>
        </head>
        <body>
            <h1>Hello World</h1>
            <p>From M5Tab5</p>
        </body>
        </html>
    )rawliteral";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// URI 路由
httpd_uri_t hello_uri = {.uri = "/", .method = HTTP_GET, .handler = hello_get_handler, .user_ctx = nullptr};

// 启动 Web Server
httpd_handle_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = nullptr;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &hello_uri);
    }
    return server;
}

// 初始化 Wi-Fi AP 模式
void wifi_init_softap()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char*>(wifi_config.ap.ssid), WIFI_SSID, sizeof(wifi_config.ap.ssid));
    std::strncpy(reinterpret_cast<char*>(wifi_config.ap.password), WIFI_PASS, sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len       = std::strlen(WIFI_SSID);
    wifi_config.ap.max_connection = MAX_STA_CONN;
    wifi_config.ap.authmode       = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi AP started. SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
}

static void wifi_ap_test_task(void* param)
{
    wifi_init_softap();
    start_webserver();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

bool HalEsp32::wifi_init()
{
    mclog::tagInfo(TAG, "wifi init");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    xTaskCreate(wifi_ap_test_task, "ap", 4096, nullptr, 5, nullptr);
    return true;
}

void HalEsp32::setExtAntennaEnable(bool enable)
{
    _ext_antenna_enable = enable;
    mclog::tagInfo(TAG, "set ext antenna enable: {}", _ext_antenna_enable);
    bsp_set_ext_antenna_enable(_ext_antenna_enable);
}

bool HalEsp32::getExtAntennaEnable()
{
    return _ext_antenna_enable;
}

void HalEsp32::startWifiAp()
{
    wifi_init();
}
