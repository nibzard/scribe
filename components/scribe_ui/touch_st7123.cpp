#include "touch_st7123.h"
#include "../scribe_hw/tab5_i2c.h"
#include "../scribe_hw/tab5_io_expander.h"
#include "mipi_dsi_display.h"
#include <driver/gpio.h>
#include <esp_lcd_io_i2c.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_touch_st7123.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
constexpr gpio_num_t kTouchIntPin = GPIO_NUM_23;
constexpr uint32_t kTouchI2cAddr = 0x55;
constexpr int kProbeTimeoutMs = 50;

static void transformPoint(uint16_t& x, uint16_t& y) {
    const auto& cfg = MIPIDSI::getConfig();
    const uint16_t w = static_cast<uint16_t>(cfg.width);
    const uint16_t h = static_cast<uint16_t>(cfg.height);
    uint16_t orig_x = x;
    uint16_t orig_y = y;

    switch (MIPIDSI::getOrientation()) {
        case MIPIDSI::Orientation::PORTRAIT:
            break;
        case MIPIDSI::Orientation::LANDSCAPE:
            x = static_cast<uint16_t>(h - 1 - orig_y);
            y = orig_x;
            break;
        case MIPIDSI::Orientation::PORTRAIT_INVERTED:
            x = static_cast<uint16_t>(w - 1 - orig_x);
            y = static_cast<uint16_t>(h - 1 - orig_y);
            break;
        case MIPIDSI::Orientation::LANDSCAPE_INVERTED:
            x = orig_y;
            y = static_cast<uint16_t>(w - 1 - orig_x);
            break;
    }
}
}

TouchST7123& TouchST7123::getInstance() {
    static TouchST7123 instance;
    return instance;
}

esp_err_t TouchST7123::init(int width, int height) {
    if (initialized_) {
        return ESP_OK;
    }

    width_ = width;
    height_ = height;

    tab5::IOExpander::getInstance().init();
    tab5::IOExpander::getInstance().setTouchReset(false);
    vTaskDelay(pdMS_TO_TICKS(5));
    tab5::IOExpander::getInstance().setTouchReset(true);

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << kTouchIntPin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
    io_conf.hys_ctrl_mode = GPIO_HYS_SOFT_DISABLE;
#endif
    gpio_config(&io_conf);

    tab5::I2CBus& bus = tab5::I2CBus::getInstance();
    esp_err_t ret = bus.init();
    if (ret != ESP_OK) {
        return ret;
    }
    ret = i2c_master_probe(bus.handle(), kTouchI2cAddr, kProbeTimeoutMs);
    if (ret != ESP_OK) {
        return ret;
    }

    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_panel_io_i2c_config_t io_cfg = {};
    io_cfg.dev_addr = kTouchI2cAddr;
    io_cfg.control_phase_bytes = 1;
    io_cfg.dc_bit_offset = 0;
    io_cfg.lcd_cmd_bits = 16;
    io_cfg.lcd_param_bits = 8;
    io_cfg.flags.disable_control_phase = 1;
    io_cfg.scl_speed_hz = 400000;

    ret = esp_lcd_new_panel_io_i2c(bus.handle(), &io_cfg, &io_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = static_cast<uint16_t>(width_),
        .y_max = static_cast<uint16_t>(height_),
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = kTouchIntPin,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    ret = esp_lcd_touch_new_i2c_st7123(io_handle, &tp_cfg, &touch_);
    if (ret != ESP_OK) {
        return ret;
    }

    initialized_ = true;
    return ESP_OK;
}

bool TouchST7123::read(lv_indev_data_t* data) {
    if (!initialized_ || !touch_) {
        return false;
    }

    esp_lcd_touch_read_data(touch_);

    uint16_t x[1] = {0};
    uint16_t y[1] = {0};
    uint16_t strength[1] = {0};
    uint8_t count = 0;
    bool touched = esp_lcd_touch_get_coordinates(touch_, x, y, strength, &count, 1);

    if (touched && count > 0) {
        uint16_t tx = x[0];
        uint16_t ty = y[0];
        transformPoint(tx, ty);
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = static_cast<int>(tx);
        data->point.y = static_cast<int>(ty);
        last_point_ = data->point;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        data->point = last_point_;
    }

    return false;
}

void TouchST7123::lvglReadCb(lv_indev_t* indev, lv_indev_data_t* data) {
    (void)indev;
    TouchST7123::getInstance().read(data);
}
