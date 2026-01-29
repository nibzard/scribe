#pragma once

#include <esp_err.h>

// Initialize TinyUSB HID keyboard device stack (Tab5 USB-C OTG).
esp_err_t initUsbHidDevice();

// Deinitialize TinyUSB HID device stack (allows re-init with different descriptors).
esp_err_t deinitUsbHidDevice();

// Send keyboard report via USB HID.
// modifier: bitfield (CTRL=1, SHIFT=2, ALT=4, GUI=8)
// keycode: up to 6 simultaneous keycodes (0 for none)
// Returns ESP_OK if sent, ESP_ERR_INVALID_STATE if not initialized
esp_err_t usbHidSendReport(uint8_t modifier, const uint8_t keycode[6]);
