#include "touch_controller.h"
#include "touch_gt911.h"
#include "touch_st7123.h"
#include "../scribe_hw/tab5_i2c.h"
#include <esp_log.h>
#include <esp_lcd_panel_io.h>

static const char* TAG = "TOUCH_CTRL";

namespace TouchController {

enum class TouchType {
    NONE,
    GT911,
    ST7123
};

static TouchType s_detected_type = TouchType::NONE;
static bool s_initialized = false;
static int s_width = 0;
static int s_height = 0;

esp_err_t init(int width, int height) {
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    s_width = width;
    s_height = height;

    // Initialize I2C bus first
    tab5::I2CBus& bus = tab5::I2CBus::getInstance();
    esp_err_t ret = bus.init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Try GT911 first (address 0x14)
    ESP_LOGI(TAG, "Probing for GT911 touch controller...");
    ret = TouchGT911::getInstance().init(width, height);
    if (ret == ESP_OK) {
        s_detected_type = TouchType::GT911;
        s_initialized = true;
        ESP_LOGI(TAG, "GT911 touch controller detected");
        return ESP_OK;
    }

    // Try ST7123 (address 0x55)
    ESP_LOGI(TAG, "Probing for ST7123 touch controller...");
    ret = TouchST7123::getInstance().init(width, height);
    if (ret == ESP_OK) {
        s_detected_type = TouchType::ST7123;
        s_initialized = true;
        ESP_LOGI(TAG, "ST7123 touch controller detected");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "No touch controller detected!");
    return ESP_ERR_NOT_FOUND;
}

lv_indev_read_cb_t getReadCallback() {
    if (!s_initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return nullptr;
    }

    switch (s_detected_type) {
        case TouchType::GT911:
            return TouchGT911::lvglReadCb;
        case TouchType::ST7123:
            return TouchST7123::lvglReadCb;
        default:
            return nullptr;
    }
}

bool isInitialized() {
    return s_initialized;
}

const char* getControllerName() {
    switch (s_detected_type) {
        case TouchType::GT911:
            return "GT911";
        case TouchType::ST7123:
            return "ST7123";
        default:
            return "None";
    }
}

} // namespace TouchController
