/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal/hal_esp32.h"
#include <mooncake_log.h>
#include <vector>
#include <driver/gpio.h>
#include <memory>
#include <mutex>
#include <lvgl.h>
#include <usb/usb_host.h>
#include <usb/hid_host.h>
#include <usb/hid_usage_keyboard.h>
#include <usb/hid_usage_mouse.h>
#include <esp_log.h>
#include <assets/assets.h>

#define TAG "usba"

static std::mutex _usba_detect_mutex;
static bool _is_usba_connected = false;
static lv_obj_t* _cursor_img;

QueueHandle_t app_event_queue = NULL;
typedef enum { APP_EVENT = 0, APP_EVENT_HID_HOST } app_event_group_t;

typedef struct {
    app_event_group_t event_group;
    struct {
        hid_host_device_handle_t handle;
        hid_host_driver_event_t event;
        void* arg;
    } hid_host_device;
} app_event_queue_t;

static const char* hid_proto_name_str[] = {"NONE", "KEYBOARD", "MOUSE"};

typedef struct {
    enum key_state { KEY_STATE_PRESSED = 0x00, KEY_STATE_RELEASED = 0x01 } state;
    uint8_t modifier;
    uint8_t key_code;
} key_event_t;

#define KEYBOARD_ENTER_MAIN_CHAR '\r'
#define KEYBOARD_ENTER_LF_EXTEND 1

static void hid_print_new_device_report_header(hid_protocol_t proto)
{
    static hid_protocol_t prev_proto_output;

    if (prev_proto_output != proto) {
        prev_proto_output = proto;
        printf("\r\n");
        if (proto == HID_PROTOCOL_MOUSE) {
            printf("Mouse\r\n");
        } else if (proto == HID_PROTOCOL_KEYBOARD) {
            printf("Keyboard\r\n");
        } else {
            printf("Generic\r\n");
        }
        fflush(stdout);
    }
}

static void hid_host_mouse_report_callback(const uint8_t* const data, const int length)
{
    hid_mouse_input_report_boot_t* mouse_report = (hid_mouse_input_report_boot_t*)data;

    if (length < sizeof(hid_mouse_input_report_boot_t)) {
        return;
    }

    static int x_pos = 720 / 2;
    static int y_pos = 1280 / 2;

    // Calculate absolute position from displacement
    x_pos += mouse_report->y_displacement;
    y_pos -= mouse_report->x_displacement;

    x_pos = std::clamp(x_pos, 0, 720);
    y_pos = std::clamp(y_pos, 0, 1280);

    hid_print_new_device_report_header(HID_PROTOCOL_MOUSE);

    // printf("X: %06d\tY: %06d\t|%c|%c|\r", x_pos, y_pos, (mouse_report->buttons.button1 ? 'o' : ' '),
    //        (mouse_report->buttons.button2 ? 'o' : ' '));

    GetHAL()->hidMouseData.mutex.lock();
    GetHAL()->hidMouseData.x        = x_pos;
    GetHAL()->hidMouseData.y        = y_pos;
    GetHAL()->hidMouseData.btnLeft  = mouse_report->buttons.button1;
    GetHAL()->hidMouseData.btnRight = mouse_report->buttons.button2;
    GetHAL()->hidMouseData.mutex.unlock();

    fflush(stdout);
}

void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle, const hid_host_interface_event_t event,
                                 void* arg)
{
    uint8_t data[64]   = {0};
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
        case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
            ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(hid_device_handle, data, 64, &data_length));

            if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
                if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                    // hid_host_keyboard_report_callback(data, data_length);
                } else if (HID_PROTOCOL_MOUSE == dev_params.proto) {
                    hid_host_mouse_report_callback(data, data_length);
                }
            } else {
                // hid_host_generic_report_callback(data, data_length);
            }

            break;
        case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HID Device, protocol '%s' DISCONNECTED", hid_proto_name_str[dev_params.proto]);
            ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));

            _usba_detect_mutex.lock();
            _is_usba_connected = false;
            _usba_detect_mutex.unlock();

            break;
        case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
            ESP_LOGI(TAG, "HID Device, protocol '%s' TRANSFER_ERROR", hid_proto_name_str[dev_params.proto]);
            break;
        default:
            ESP_LOGE(TAG, "HID Device, protocol '%s' Unhandled event", hid_proto_name_str[dev_params.proto]);
            break;
    }
}

void hid_host_device_event(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void* arg)
{
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
        case HID_HOST_DRIVER_EVENT_CONNECTED: {
            ESP_LOGI(TAG, "HID Device, protocol '%s' CONNECTED", hid_proto_name_str[dev_params.proto]);

            const hid_host_device_config_t dev_config = {.callback = hid_host_interface_callback, .callback_arg = NULL};

            ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
            if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
                ESP_ERROR_CHECK(hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT));
                if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                    ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
                }
            }
            ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));

            _usba_detect_mutex.lock();
            _is_usba_connected = true;
            _usba_detect_mutex.unlock();

            break;
        }
        default:
            break;
    }
}

void hid_host_device_callback(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event,
                              void* arg)
{
    const app_event_queue_t evt_queue = {.event_group = APP_EVENT_HID_HOST,
                                         // HID Host Device related info
                                         .hid_host_device = {.handle = hid_device_handle, .event = event, .arg = arg}};

    if (app_event_queue) {
        xQueueSend(app_event_queue, &evt_queue, 0);
    }
}

static void tab5_usb_host_task(void* pvParameters)
{
    // BaseType_t task_created;
    app_event_queue_t evt_queue;
    // ESP_LOGI(TAG, "HID Host example");
    // task_created = xTaskCreatePinnedToCore(usb_lib_task, "usb_events", 4096, xTaskGetCurrentTaskHandle(), 2, NULL,
    // 0); assert(task_created == pdTRUE);

    ulTaskNotifyTake(false, 1000);
    const hid_host_driver_config_t hid_host_driver_config = {.create_background_task = true,
                                                             .task_priority          = 5,
                                                             .stack_size             = 4096,
                                                             .core_id                = 0,
                                                             .callback               = hid_host_device_callback,
                                                             .callback_arg           = NULL};

    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

    // Create queue
    app_event_queue = xQueueCreate(10, sizeof(app_event_queue_t));

    ESP_LOGI(TAG, "Waiting for HID Device to be connected");

    while (1) {
        // Wait queue
        if (xQueueReceive(app_event_queue, &evt_queue, portMAX_DELAY)) {
            if (APP_EVENT == evt_queue.event_group) {
                // User pressed button
                usb_host_lib_info_t lib_info;
                ESP_ERROR_CHECK(usb_host_lib_info(&lib_info));
                if (lib_info.num_devices == 0) {
                    // End while cycle
                    break;
                } else {
                    ESP_LOGW(TAG, "To shutdown example, remove all USB devices and press button again.");
                    // Keep polling
                }
            }

            if (APP_EVENT_HID_HOST == evt_queue.event_group) {
                hid_host_device_event(evt_queue.hid_host_device.handle, evt_queue.hid_host_device.event,
                                      evt_queue.hid_host_device.arg);
            }
        }
    }
}

static void lvgl_mouse_read_cb(lv_indev_t* indev, lv_indev_data_t* data)
{
    _usba_detect_mutex.lock();
    if (!_is_usba_connected) {
        _usba_detect_mutex.unlock();
        data->state = LV_INDEV_STATE_REL;
        if (lv_obj_get_style_opa(_cursor_img, LV_PART_MAIN) == LV_OPA_COVER) {
            lv_obj_set_style_opa(_cursor_img, LV_OPA_TRANSP, LV_PART_MAIN);
        }
        return;
    }
    _usba_detect_mutex.unlock();
    if (lv_obj_get_style_opa(_cursor_img, LV_PART_MAIN) == LV_OPA_TRANSP) {
        lv_obj_set_style_opa(_cursor_img, LV_OPA_COVER, LV_PART_MAIN);
    }

    std::lock_guard<std::mutex> lock(GetHAL()->hidMouseData.mutex);
    data->point.x = GetHAL()->hidMouseData.x;
    data->point.y = GetHAL()->hidMouseData.y;
    data->state   = GetHAL()->hidMouseData.btnLeft ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void HalEsp32::hid_init()
{
    mclog::tagInfo(TAG, "hid init");
    xTaskCreatePinnedToCore(tab5_usb_host_task, "usba", 4096 * 2, NULL, 5, NULL, 0);

    auto lvMouse = lv_indev_create();
    lv_indev_set_type(lvMouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvMouse, lvgl_mouse_read_cb);
    lv_indev_set_display(lvMouse, lvDisp);

    _cursor_img = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
    lv_image_set_src(_cursor_img, &mouse_cursor);      /*Set the image source*/
    lv_indev_set_cursor(lvMouse, _cursor_img);         /*Connect the image  object to the driver*/
}

bool HalEsp32::usbADetect()
{
    std::lock_guard<std::mutex> lock(_usba_detect_mutex);
    return _is_usba_connected;
}
