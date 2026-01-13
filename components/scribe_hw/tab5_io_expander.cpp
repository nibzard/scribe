#include "tab5_io_expander.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace tab5 {

namespace {
constexpr uint8_t kAddrIo0 = 0x43;
constexpr uint8_t kAddrIo1 = 0x44;

constexpr uint8_t kRegOutSet = 0x05;
constexpr uint8_t kRegIoDir = 0x03;
constexpr uint8_t kRegOutHighZ = 0x07;
constexpr uint8_t kRegPullSel = 0x0D;
constexpr uint8_t kRegPullEn = 0x0B;
constexpr uint8_t kRegInDefSta = 0x09;
constexpr uint8_t kRegIntMask = 0x11;
constexpr uint8_t kRegInput = 0x0F;

constexpr uint8_t kBitWifiAntennaSwitch = 1 << 0;
constexpr uint8_t kBitSpeakerEnable = 1 << 1;
constexpr uint8_t kBitExt5VEnable = 1 << 2;
constexpr uint8_t kBitLcdReset = 1 << 4;
constexpr uint8_t kBitTouchReset = 1 << 5;
constexpr uint8_t kBitUsb5VEnable = 1 << 3;
constexpr uint8_t kBitChargeEnable = 1 << 7;
constexpr uint8_t kBitChargeStatus = 1 << 6;
constexpr uint8_t kBitPowerOffPulse = 1 << 4;
constexpr uint8_t kBitWlanPower = 1 << 0;

constexpr uint8_t kOutInitIo0 = 0b01110110;
constexpr uint8_t kOutInitIo1 = 0b10001001;

constexpr uint8_t kRegArrayIo0[] = {
    kRegOutSet, kOutInitIo0,
    kRegIoDir,  0b01111111,
    kRegOutHighZ, 0b00000000,
    kRegPullSel, 0b01111111,
    kRegPullEn,  0b01111111,
};

constexpr uint8_t kRegArrayIo1[] = {
    kRegOutSet, kOutInitIo1,
    kRegIoDir,  0b10111001,
    kRegOutHighZ, 0b00000110,
    kRegPullSel, 0b10111001,
    kRegPullEn,  0b11111001,
    kRegInDefSta, 0b01000000,
    kRegIntMask, 0b10111111,
};
}

IOExpander& IOExpander::getInstance() {
    static IOExpander instance;
    return instance;
}

esp_err_t IOExpander::init() {
    if (initialized_) {
        return ESP_OK;
    }

    esp_err_t ret = io0_.init(kAddrIo0);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = io1_.init(kAddrIo1);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = writeRegisterArray(0, kRegArrayIo0, sizeof(kRegArrayIo0));
    if (ret != ESP_OK) {
        return ret;
    }
    ret = writeRegisterArray(1, kRegArrayIo1, sizeof(kRegArrayIo1));
    if (ret != ESP_OK) {
        return ret;
    }

    out_cache_[0] = kOutInitIo0;
    out_cache_[1] = kOutInitIo1;
    initialized_ = true;
    return ESP_OK;
}

esp_err_t IOExpander::writeRegisterArray(int index, const uint8_t* data, size_t len) {
    if (!data || len % 2 != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    I2CDevice& dev = (index == 0) ? io0_ : io1_;
    if (!dev.isInitialized()) {
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t i = 0; i < len; i += 2) {
        esp_err_t ret = dev.writeReg8(data[i], data[i + 1]);
        if (ret != ESP_OK) {
            return ret;
        }
    }
    return ESP_OK;
}

esp_err_t IOExpander::updateOutput(int index, uint8_t mask, bool level) {
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }
    if (index < 0 || index > 1) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t value = out_cache_[index];
    if (level) {
        value |= mask;
    } else {
        value &= static_cast<uint8_t>(~mask);
    }

    I2CDevice& dev = (index == 0) ? io0_ : io1_;
    esp_err_t ret = dev.writeReg8(kRegOutSet, value);
    if (ret == ESP_OK) {
        out_cache_[index] = value;
    }
    return ret;
}

esp_err_t IOExpander::readInput(int index, uint8_t* value) {
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    I2CDevice& dev = (index == 0) ? io0_ : io1_;
    if (!dev.isInitialized()) {
        return ESP_ERR_INVALID_STATE;
    }

    return dev.readReg(kRegInput, value, 1);
}

esp_err_t IOExpander::setLcdReset(bool level) {
    return updateOutput(0, kBitLcdReset, level);
}

esp_err_t IOExpander::setTouchReset(bool level) {
    return updateOutput(0, kBitTouchReset, level);
}

esp_err_t IOExpander::resetDisplayAndTouch() {
    esp_err_t ret = setLcdReset(false);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = setTouchReset(false);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(5));
    ret = setLcdReset(true);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = setTouchReset(true);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}

esp_err_t IOExpander::setUsb5VEnable(bool enable) {
    return updateOutput(1, kBitUsb5VEnable, enable);
}

esp_err_t IOExpander::setExt5VEnable(bool enable) {
    return updateOutput(0, kBitExt5VEnable, enable);
}

esp_err_t IOExpander::setSpeakerEnable(bool enable) {
    return updateOutput(0, kBitSpeakerEnable, enable);
}

esp_err_t IOExpander::setWlanPower(bool enable) {
    return updateOutput(1, kBitWlanPower, enable);
}

esp_err_t IOExpander::setWifiAntennaExternal(bool external) {
    return updateOutput(0, kBitWifiAntennaSwitch, external);
}

bool IOExpander::isCharging() const {
    uint8_t input = 0;
    if (const_cast<IOExpander*>(this)->readInput(1, &input) != ESP_OK) {
        return false;
    }
    return (input & kBitChargeStatus) != 0;
}

esp_err_t IOExpander::setChargeEnable(bool enable) {
    return updateOutput(1, kBitChargeEnable, enable);
}

esp_err_t IOExpander::pulsePowerOff() {
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }
    for (int i = 0; i < 10; ++i) {
        esp_err_t ret = updateOutput(1, kBitPowerOffPulse, (i & 1) != 0);
        if (ret != ESP_OK) {
            return ret;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    return ESP_OK;
}

} // namespace tab5
