#include "tab5_i2c.h"
#include <vector>

namespace tab5 {

namespace {
constexpr int kI2CTimeoutMs = 100;
}

I2CBus& I2CBus::getInstance() {
    static I2CBus instance;
    return instance;
}

esp_err_t I2CBus::init(gpio_num_t sda, gpio_num_t scl) {
    if (initialized_ || bus_ != nullptr) {
        initialized_ = true;
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = I2C_NUM_0;
    bus_config.sda_io_num = sda;
    bus_config.scl_io_num = scl;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.intr_priority = 0;
    bus_config.trans_queue_depth = 0;
    bus_config.flags.enable_internal_pullup = true;
    bus_config.flags.allow_pd = false;

    esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_);
    if (ret == ESP_OK) {
        initialized_ = true;
    }
    return ret;
}

esp_err_t I2CBus::adopt(i2c_master_bus_handle_t bus) {
    if (!bus) {
        return ESP_ERR_INVALID_ARG;
    }
    if (initialized_) {
        return (bus_ == bus) ? ESP_OK : ESP_ERR_INVALID_STATE;
    }
    bus_ = bus;
    initialized_ = true;
    return ESP_OK;
}

esp_err_t I2CDevice::init(uint8_t address, uint32_t freq_hz) {
    if (dev_) {
        return ESP_OK;
    }
    I2CBus& bus = I2CBus::getInstance();
    esp_err_t ret = bus.init();
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_device_config_t dev_config = {};
    dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_config.device_address = address;
    dev_config.scl_speed_hz = freq_hz;
    dev_config.scl_wait_us = 0;
    dev_config.flags.disable_ack_check = false;

    return i2c_master_bus_add_device(bus.handle(), &dev_config, &dev_);
}

esp_err_t I2CDevice::write(const uint8_t* data, size_t len) {
    if (!dev_) {
        return ESP_ERR_INVALID_STATE;
    }
    return i2c_master_transmit(dev_, data, len, kI2CTimeoutMs);
}

esp_err_t I2CDevice::read(uint8_t* data, size_t len) {
    if (!dev_) {
        return ESP_ERR_INVALID_STATE;
    }
    return i2c_master_receive(dev_, data, len, kI2CTimeoutMs);
}

esp_err_t I2CDevice::writeReg8(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return write(buf, sizeof(buf));
}

esp_err_t I2CDevice::writeReg(uint8_t reg, const uint8_t* data, size_t len) {
    if (!dev_) {
        return ESP_ERR_INVALID_STATE;
    }
    if (len == 0) {
        return writeReg8(reg, 0);
    }
    std::vector<uint8_t> buf(len + 1);
    buf[0] = reg;
    for (size_t i = 0; i < len; ++i) {
        buf[1 + i] = data[i];
    }
    return i2c_master_transmit(dev_, buf.data(), buf.size(), kI2CTimeoutMs);
}

esp_err_t I2CDevice::readReg(uint8_t reg, uint8_t* data, size_t len) {
    if (!dev_) {
        return ESP_ERR_INVALID_STATE;
    }
    return i2c_master_transmit_receive(dev_, &reg, 1, data, len, kI2CTimeoutMs);
}

esp_err_t I2CDevice::writeReg16(uint16_t reg, const uint8_t* data, size_t len) {
    if (!dev_) {
        return ESP_ERR_INVALID_STATE;
    }
    std::vector<uint8_t> buf(len + 2);
    buf[0] = static_cast<uint8_t>((reg >> 8) & 0xFF);
    buf[1] = static_cast<uint8_t>(reg & 0xFF);
    for (size_t i = 0; i < len; ++i) {
        buf[2 + i] = data[i];
    }
    return i2c_master_transmit(dev_, buf.data(), buf.size(), kI2CTimeoutMs);
}

esp_err_t I2CDevice::readReg16(uint16_t reg, uint8_t* data, size_t len) {
    if (!dev_) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t buf[2] = {
        static_cast<uint8_t>((reg >> 8) & 0xFF),
        static_cast<uint8_t>(reg & 0xFF)
    };
    return i2c_master_transmit_receive(dev_, buf, sizeof(buf), data, len, kI2CTimeoutMs);
}

esp_err_t I2CDevice::updateRegBits(uint8_t reg, uint8_t mask, bool set) {
    uint8_t value = 0;
    esp_err_t ret = readReg(reg, &value, 1);
    if (ret != ESP_OK) {
        return ret;
    }
    if (set) {
        value |= mask;
    } else {
        value &= static_cast<uint8_t>(~mask);
    }
    return writeReg8(reg, value);
}

} // namespace tab5
