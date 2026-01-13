#pragma once

#include "tab5_i2c.h"
#include <ctime>

namespace tab5 {

class Rx8130 {
public:
    static constexpr uint8_t kAddress = 0x32;

    bool init();
    bool isInitialized() const { return initialized_; }

    bool readTime(struct tm* out);
    bool writeTime(const struct tm& time);

private:
    static uint8_t bcdToDec(uint8_t value);
    static uint8_t decToBcd(uint8_t value);
    esp_err_t updateBits(uint8_t reg, uint8_t mask, bool set);

    I2CDevice dev_{};
    bool initialized_{false};
};

} // namespace tab5
