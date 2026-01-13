#pragma once

#include "tab5_i2c.h"
#include <cstdint>

namespace tab5 {

class Ina226 {
public:
    static constexpr uint8_t kAddress = 0x40;

    enum class Sampling : uint8_t {
        Rate1 = 0b000,
        Rate4 = 0b001,
        Rate16 = 0b010,
        Rate64 = 0b011,
        Rate128 = 0b100,
        Rate256 = 0b101,
        Rate512 = 0b110,
        Rate1024 = 0b111,
    };

    enum class ConversionTime : uint8_t {
        Us140 = 0b000,
        Us204 = 0b001,
        Us332 = 0b010,
        Us588 = 0b011,
        Us1100 = 0b100,
        Us2116 = 0b101,
        Us4156 = 0b110,
        Us8244 = 0b111,
    };

    enum class Mode : uint8_t {
        PowerDown = 0b000,
        ShuntVoltageSingle = 0b001,
        BusVoltageSingle = 0b010,
        ShuntAndBusSingle = 0b011,
        ShuntVoltage = 0b101,
        BusVoltage = 0b110,
        ShuntAndBus = 0b111,
    };

    struct Config {
        float shunt_res = 0.1f;
        float max_expected_current = 2.0f;
        Sampling sampling_rate = Sampling::Rate16;
        ConversionTime shunt_conversion_time = ConversionTime::Us1100;
        ConversionTime bus_conversion_time = ConversionTime::Us1100;
        Mode mode = Mode::ShuntAndBus;
    };

    bool init();
    void configure(const Config& cfg);

    float getBusVoltage();
    float getShuntCurrent();

private:
    uint16_t readRegister16(uint8_t reg);
    esp_err_t writeRegister16(uint8_t reg, uint16_t value);

    I2CDevice dev_{};
    bool initialized_{false};
    float shunt_res_{0.1f};
    float cur_lsb_{0.1f};
};

} // namespace tab5
