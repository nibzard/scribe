#include "usb_hid_device.h"
#include "sdkconfig.h"
#include <esp_log.h>

#if defined(CONFIG_TINYUSB_HID_COUNT) && (CONFIG_TINYUSB_HID_COUNT > 0)
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"

static const char* TAG = "SCRIBE_USB_HID";

static bool s_initialized = false;

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

static const char kLangId[] = {0x09, 0x04};

static const uint8_t kHidReportDescriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

static const char* kHidStringDescriptor[] = {
    kLangId,  // 0: English (0x0409)
    CONFIG_TINYUSB_DESC_MANUFACTURER_STRING,
    CONFIG_TINYUSB_DESC_PRODUCT_STRING,
    CONFIG_TINYUSB_DESC_SERIAL_STRING,
    "Scribe HID Keyboard",
};

static const uint8_t kHidConfigurationDescriptor[] = {
    // Configuration number, interface count, string index, total length, attributes, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, true, sizeof(kHidReportDescriptor), 0x81, 16, 10),
};

extern "C" {

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return kHidReportDescriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t* buffer, uint16_t reqlen) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const* buffer, uint16_t bufsize) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

} // extern "C"

esp_err_t initUsbHidDevice() {
    if (s_initialized) {
        return ESP_OK;
    }

    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = NULL;
    tusb_cfg.descriptor.full_speed_config = kHidConfigurationDescriptor;
    tusb_cfg.descriptor.string = kHidStringDescriptor;
    tusb_cfg.descriptor.string_count = sizeof(kHidStringDescriptor) / sizeof(kHidStringDescriptor[0]);
#if TUD_OPT_HIGH_SPEED
    tusb_cfg.descriptor.high_speed_config = kHidConfigurationDescriptor;
#endif

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_initialized = true;
    return ESP_OK;
}

esp_err_t usbHidSendReport(uint8_t modifier, const uint8_t keycode[6]) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Build HID report: modifier, reserved, 6 keycodes
    uint8_t report[8] = {modifier, 0};
    if (keycode) {
        for (int i = 0; i < 6; i++) {
            report[2 + i] = keycode[i] ? keycode[i] : 0;
        }
    }

    // Send report (tinyusb task runs automatically)
    if (!tud_hid_ready()) {
        return ESP_ERR_INVALID_STATE;
    }

    tud_hid_report(0, report, sizeof(report));
    return ESP_OK;
}

#else
esp_err_t initUsbHidDevice() {
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t usbHidSendReport(uint8_t modifier, const uint8_t keycode[6]) {
    return ESP_ERR_NOT_SUPPORTED;
}
#endif
