#include "display_driver.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <cstring>
#include <algorithm>

static const char* TAG = "SCRIBE_DISPLAY";

// Display state
static struct {
    lv_display_t* display;
    int width;
    int height;
    int rotation;
    bool double_buffer;
    bool initialized;

    // Draw buffers
    lv_draw_buf_t* dbuf1;
    lv_draw_buf_t* dbuf2;

    // Hardware interface
    void* hw_handle;  // Platform-specific handle

    // Semaphore for flush operations
    SemaphoreHandle_t flush_sem;
} state = {
    .display = nullptr,
    .width = 320,
    .height = 240,
    .rotation = 0,
    .double_buffer = false,
    .initialized = false,
    .dbuf1 = nullptr,
    .dbuf2 = nullptr,
    .hw_handle = nullptr,
    .flush_sem = nullptr
};

// Forward declarations
static void flushReady(lv_display_t* display);

// LVGL flush callback
extern "C" void lvgl_flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* px_map) {
    ESP_LOGD(TAG, "Flush request: area (%d,%d) to (%d,%d)",
             area->x1, area->y1, area->x2, area->y2);

    // TODO: Implement hardware-specific flush
    // For MIPI-DSI: Send data to display controller
    // For SPI: Send data via SPI interface
    // For RGB: Copy to framebuffer

    // Example for hardware implementation:
    // 1. Calculate offset in framebuffer
    // 2. Convert color format if needed (LVGL -> native)
    // 3. Send to display hardware

    // For now, just notify LVGL that we're done
    // This makes the UI work on simulators/emulators
    lv_display_flush_ready(display);
}

esp_err_t DisplayDriver::init(const DisplayConfig& config) {
    if (state.initialized) {
        ESP_LOGW(TAG, "Display driver already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing display driver: %dx%d, rotation=%d",
             config.width, config.height, config.rotation);

    state.width = config.width;
    state.height = config.height;
    state.rotation = config.rotation;
    state.double_buffer = config.double_buffer;

    // Create flush semaphore
    state.flush_sem = xSemaphoreCreateBinary();
    if (!state.flush_sem) {
        ESP_LOGE(TAG, "Failed to create flush semaphore");
        return ESP_ERR_NO_MEM;
    }

    // Calculate buffer size
    int hor_res = config.width;
    int ver_res = config.height;

    if (config.rotation == 90 || config.rotation == 270) {
        std::swap(hor_res, ver_res);
    }

    // Buffer height: either full screen or specified line count
    int lines = (config.buffer_size > 0) ? config.buffer_size : ver_res;
    state.dbuf1 = lv_draw_buf_create(hor_res, lines, LV_COLOR_FORMAT_RGB565, 0);
    if (!state.dbuf1) {
        ESP_LOGE(TAG, "Failed to create LVGL draw buffer 1");
        vSemaphoreDelete(state.flush_sem);
        state.flush_sem = nullptr;
        return ESP_ERR_NO_MEM;
    }
    if (config.double_buffer) {
        state.dbuf2 = lv_draw_buf_create(hor_res, lines, LV_COLOR_FORMAT_RGB565, 0);
        if (!state.dbuf2) {
            ESP_LOGW(TAG, "Failed to create second draw buffer, using single buffer");
            state.double_buffer = false;
        }
    }

    // Create LVGL display
    state.display = lv_display_create(hor_res, ver_res);
    if (!state.display) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        heap_caps_free(state.buf1);
        if (state.buf2) heap_caps_free(state.buf2);
        vSemaphoreDelete(state.flush_sem);
        return ESP_ERR_NO_MEM;
    }

    // Set flush callback
    lv_display_set_flush_cb(state.display, lvgl_flush_cb);

    // Set draw buffers
    lv_display_set_draw_buffers(state.display, state.dbuf1, state.double_buffer ? state.dbuf2 : nullptr);

    // Set default color format
    lv_display_set_color_format(state.display, LV_COLOR_FORMAT_RGB565);

    ESP_LOGI(TAG, "Display driver initialized successfully");
    ESP_LOGI(TAG, "  Resolution: %dx%d", hor_res, ver_res);
    ESP_LOGI(TAG, "  Buffering: %s", state.double_buffer ? "double" : "single");

    state.initialized = true;
    return ESP_OK;
}

esp_err_t DisplayDriver::setRotation(int rotation_degrees) {
    if (!state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Validate rotation
    if (rotation_degrees % 90 != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    rotation_degrees = rotation_degrees % 360;

    ESP_LOGI(TAG, "Setting rotation to %d degrees", rotation_degrees);

    // Swap dimensions if rotating 90 or 270
    if ((rotation_degrees == 90 || rotation_degrees == 270) !=
        (state.rotation == 90 || state.rotation == 270)) {
        std::swap(state.width, state.height);
    }

    state.rotation = rotation_degrees;

    // Update LVGL display size
    int hor_res = state.width;
    int ver_res = state.height;

    if (rotation_degrees == 90 || rotation_degrees == 270) {
        std::swap(hor_res, ver_res);
    }

    lv_display_set_resolution(state.display, hor_res, ver_res);

    // TODO: Send rotation command to hardware
    // Example: mipi_dsi_set_rotation(hw_handle, rotation_degrees);

    return ESP_OK;
}

void DisplayDriver::getDimensions(int* width, int* height) {
    if (width) *width = state.width;
    if (height) *height = state.height;
}

esp_err_t DisplayDriver::setBacklight(int percent) {
    if (percent < 0 || percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Setting backlight to %d%%", percent);

    // TODO: Implement hardware backlight control
    // Example for PWM backlight:
    //     ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, percent * 8191 / 100);
    //     ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // Example for GPIO backlight (on/off only):
    //     gpio_set_level(BACKLIGHT_GPIO, percent > 0 ? 1 : 0);

    return ESP_OK;
}

esp_err_t DisplayDriver::displayOn(bool on) {
    ESP_LOGI(TAG, "Turning display %s", on ? "on" : "off");

    // TODO: Implement hardware display power control
    // Example:
    //     gpio_set_level(DISPLAY_PWR_GPIO, on ? 1 : 0);
    //     vTaskDelay(pdMS_TO_TICKS(50));  // Wait for power rail to stabilize
    //     mipi_dsi_send_cmd(on ? MIPI_DSI_CMD_ON : MIPI_DSI_CMD_OFF);

    return ESP_OK;
}

void DisplayDriver::deinit() {
    if (!state.initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing display driver");

    if (state.display) {
        lv_display_delete(state.display);
        state.display = nullptr;
    }

    if (state.dbuf1) {
        lv_draw_buf_destroy(state.dbuf1);
        state.dbuf1 = nullptr;
    }
    if (state.dbuf2) {
        lv_draw_buf_destroy(state.dbuf2);
        state.dbuf2 = nullptr;
    }

    if (state.flush_sem) {
        vSemaphoreDelete(state.flush_sem);
        state.flush_sem = nullptr;
    }

    state.initialized = false;
}

lv_display_t* DisplayDriver::getDisplay() {
    return state.display;
}
