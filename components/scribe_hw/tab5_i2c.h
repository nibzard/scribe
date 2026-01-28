#pragma once

#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <esp_err.h>
#include <cstddef>
#include <cstdint>

namespace tab5 {

class I2CBus {
public:
    static I2CBus& getInstance();

    esp_err_t init(gpio_num_t sda = GPIO_NUM_31,
                   gpio_num_t scl = GPIO_NUM_32);

    esp_err_t adopt(i2c_master_bus_handle_t bus);

    bool isInitialized() const { return initialized_; }

    i2c_master_bus_handle_t handle() const { return bus_; }

private:
    I2CBus() = default;

    i2c_master_bus_handle_t bus_{nullptr};
    bool initialized_{false};
};

class I2CDevice {
public:
    esp_err_t init(uint8_t address, uint32_t freq_hz = 400000);
    bool isInitialized() const { return dev_ != nullptr; }

    esp_err_t write(const uint8_t* data, size_t len);
    esp_err_t read(uint8_t* data, size_t len);

    esp_err_t writeReg8(uint8_t reg, uint8_t value);
    esp_err_t writeReg(uint8_t reg, const uint8_t* data, size_t len);
    esp_err_t readReg(uint8_t reg, uint8_t* data, size_t len);

    esp_err_t writeReg16(uint16_t reg, const uint8_t* data, size_t len);
    esp_err_t readReg16(uint16_t reg, uint8_t* data, size_t len);

    esp_err_t updateRegBits(uint8_t reg, uint8_t mask, bool set);

private:
    i2c_master_dev_handle_t dev_{nullptr};
};

} // namespace tab5
