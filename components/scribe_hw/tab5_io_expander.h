#pragma once

#include "tab5_i2c.h"
#include <esp_err.h>
#include <cstddef>
#include <cstdint>

namespace tab5 {

class IOExpander {
public:
    static IOExpander& getInstance();

    esp_err_t init();
    bool isInitialized() const { return initialized_; }

    esp_err_t setLcdReset(bool level);
    esp_err_t setTouchReset(bool level);
    esp_err_t resetDisplayAndTouch();

    esp_err_t setUsb5VEnable(bool enable);
    esp_err_t setExt5VEnable(bool enable);
    esp_err_t setSpeakerEnable(bool enable);
    esp_err_t setWlanPower(bool enable);
    esp_err_t setWifiAntennaExternal(bool external);

    bool isCharging() const;
    esp_err_t setChargeEnable(bool enable);
    esp_err_t pulsePowerOff();

private:
    IOExpander() = default;

    esp_err_t writeRegisterArray(int index, const uint8_t* data, size_t len);
    esp_err_t updateOutput(int index, uint8_t mask, bool level);
    esp_err_t readInput(int index, uint8_t* value);

    I2CDevice io0_{};
    I2CDevice io1_{};
    bool initialized_{false};
    uint8_t out_cache_[2] = {0, 0};
};

} // namespace tab5
