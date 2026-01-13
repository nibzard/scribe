#include "tab5_ina226.h"
namespace tab5 {

namespace {
constexpr uint8_t kRegConfig = 0x00;
constexpr uint8_t kRegBusVoltage = 0x02;
constexpr uint8_t kRegCurrent = 0x04;
constexpr uint8_t kRegCalibration = 0x05;
constexpr uint8_t kRegDeviceId = 0xFF;
constexpr uint16_t kExpectedDeviceId = 0x2260;
}

bool Ina226::init() {
    if (initialized_) {
        return true;
    }
    if (dev_.init(kAddress) != ESP_OK) {
        return false;
    }
    uint16_t id = readRegister16(kRegDeviceId);
    initialized_ = (id == kExpectedDeviceId);
    return initialized_;
}

void Ina226::configure(const Config& cfg) {
    uint16_t value = (static_cast<uint16_t>(cfg.sampling_rate) << 9)
                   | (static_cast<uint16_t>(cfg.bus_conversion_time) << 6)
                   | (static_cast<uint16_t>(cfg.shunt_conversion_time) << 3)
                   | static_cast<uint16_t>(cfg.mode);

    writeRegister16(kRegConfig, value);

    float current_lsb = cfg.max_expected_current / 32768.0f;
    cur_lsb_ = current_lsb;
    shunt_res_ = cfg.shunt_res;

    uint16_t calibration = static_cast<uint16_t>(0.00512f / (current_lsb * cfg.shunt_res));
    writeRegister16(kRegCalibration, calibration);
}

float Ina226::getBusVoltage() {
    int16_t raw = static_cast<int16_t>(readRegister16(kRegBusVoltage));
    return raw * 0.00125f;
}

float Ina226::getShuntCurrent() {
    int16_t raw = static_cast<int16_t>(readRegister16(kRegCurrent));
    return raw * cur_lsb_;
}

uint16_t Ina226::readRegister16(uint8_t reg) {
    uint8_t buf[2] = {0, 0};
    if (dev_.readReg(reg, buf, sizeof(buf)) != ESP_OK) {
        return 0;
    }
    return static_cast<uint16_t>(buf[0] << 8 | buf[1]);
}

esp_err_t Ina226::writeRegister16(uint8_t reg, uint16_t value) {
    uint8_t buf[2] = {
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>(value & 0xFF)
    };
    return dev_.writeReg(reg, buf, sizeof(buf));
}

} // namespace tab5
