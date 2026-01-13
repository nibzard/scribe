#pragma once

#include <esp_err.h>
#include <lvgl.h>
#include <esp_lcd_touch.h>

class TouchGT911 {
public:
    static TouchGT911& getInstance();

    esp_err_t init(int width, int height);
    bool read(lv_indev_data_t* data);

    static void lvglReadCb(lv_indev_t* indev, lv_indev_data_t* data);

private:
    TouchGT911() = default;

    esp_lcd_touch_handle_t touch_{nullptr};
    bool initialized_{false};
    int width_{0};
    int height_{0};
    lv_point_t last_point_{};
};
