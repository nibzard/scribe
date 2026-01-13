#include "tab5_rx8130.h"

namespace tab5 {

namespace {
constexpr uint8_t kRegSeconds = 0x10;
constexpr uint8_t kRegWeekDay = 0x13;
constexpr uint8_t kRegControl = 0x1E;
constexpr uint8_t kRegFlag = 0x1F;
constexpr uint8_t kRegExt = 0x30;
}

uint8_t Rx8130::bcdToDec(uint8_t value) {
    return static_cast<uint8_t>(((value >> 4) * 10) + (value & 0x0F));
}

uint8_t Rx8130::decToBcd(uint8_t value) {
    uint8_t high = value / 10;
    return static_cast<uint8_t>((high << 4) | (value - (high * 10)));
}

esp_err_t Rx8130::updateBits(uint8_t reg, uint8_t mask, bool set) {
    uint8_t value = 0;
    esp_err_t ret = dev_.readReg(reg, &value, 1);
    if (ret != ESP_OK) {
        return ret;
    }
    if (set) {
        value |= mask;
    } else {
        value &= static_cast<uint8_t>(~mask);
    }
    return dev_.writeReg8(reg, value);
}

bool Rx8130::init() {
    if (initialized_) {
        return true;
    }
    if (dev_.init(kAddress) != ESP_OK) {
        return false;
    }

    bool ok = (updateBits(kRegFlag, 0x30, true) == ESP_OK);
    ok = ok && (dev_.writeReg8(kRegExt, 0x00) == ESP_OK);
    ok = ok && (dev_.writeReg8(kRegControl, 0x00) == ESP_OK);

    initialized_ = ok;
    return initialized_;
}

bool Rx8130::readTime(struct tm* out) {
    if (!initialized_ || !out) {
        return false;
    }

    uint8_t buf[7] = {0};
    if (dev_.readReg(kRegSeconds, buf, sizeof(buf)) != ESP_OK) {
        return false;
    }

    out->tm_sec = bcdToDec(buf[0] & 0x7F);
    out->tm_min = bcdToDec(buf[1] & 0x7F);
    out->tm_hour = bcdToDec(buf[2] & 0x3F);

    uint8_t weekday_bits = buf[3];
    out->tm_wday = (weekday_bits == 0) ? 0 : __builtin_ctz(weekday_bits);

    out->tm_mday = bcdToDec(buf[4] & 0x3F);
    out->tm_mon = bcdToDec(buf[5] & 0x1F) - 1;
    out->tm_year = bcdToDec(buf[6]) + 100;
    out->tm_isdst = 0;

    return true;
}

bool Rx8130::writeTime(const struct tm& time) {
    if (!initialized_) {
        return false;
    }

    uint8_t buf[7] = {0};
    buf[0] = decToBcd(static_cast<uint8_t>(time.tm_sec));
    buf[1] = decToBcd(static_cast<uint8_t>(time.tm_min));
    buf[2] = decToBcd(static_cast<uint8_t>(time.tm_hour));
    buf[3] = static_cast<uint8_t>(1u << (time.tm_wday & 0x07));
    buf[4] = decToBcd(static_cast<uint8_t>(time.tm_mday));
    buf[5] = decToBcd(static_cast<uint8_t>(time.tm_mon + 1));
    buf[6] = decToBcd(static_cast<uint8_t>((time.tm_year + 1900) % 100));

    return dev_.writeReg(kRegSeconds, buf, sizeof(buf)) == ESP_OK;
}

} // namespace tab5
