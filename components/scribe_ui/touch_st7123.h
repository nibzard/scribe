#pragma once

#include <esp_err.h>
#include <esp_lcd_touch.h>
#include <lvgl.h>

class TouchST7123 {
public:
    static TouchST7123& getInstance();

    esp_err_t init(int width, int height);
    bool read(lv_indev_data_t* data);

    static void lvglReadCb(lv_indev_t* indev, lv_indev_data_t* data);

private:
    TouchST7123() = default;

    esp_lcd_touch_handle_t touch_{nullptr};
    bool initialized_{false};
    int width_{0};
    int height_{0};
    lv_point_t last_point_{};
};
