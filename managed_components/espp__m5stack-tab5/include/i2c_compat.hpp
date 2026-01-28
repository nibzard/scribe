#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <system_error>
#include <unordered_map>

#include <driver/gpio.h>

#include "i2c_master.hpp"

namespace espp {

// Compatibility wrapper to provide the legacy espp::I2c interface using the new driver_ng API.
class I2c {
public:
  struct Config {
    int isr_core_id = -1;
    i2c_port_t port = I2C_NUM_0;
    gpio_num_t sda_io_num = GPIO_NUM_NC;
    gpio_num_t scl_io_num = GPIO_NUM_NC;
    gpio_pullup_t sda_pullup_en = GPIO_PULLUP_DISABLE;
    gpio_pullup_t scl_pullup_en = GPIO_PULLUP_DISABLE;
    uint32_t timeout_ms = 10;
    uint32_t clk_speed = 400 * 1000;
    bool auto_init = true;
    Logger::Verbosity log_level = Logger::Verbosity::WARN;
  };

  explicit I2c(const Config &config)
      : config_(config)
      , bus_(I2cMasterBus::Config{
            .port = config.port,
            .sda_io_num = config.sda_io_num,
            .scl_io_num = config.scl_io_num,
            .clk_speed = config.clk_speed,
            .enable_internal_pullup = (config.sda_pullup_en == GPIO_PULLUP_ENABLE) ||
                                      (config.scl_pullup_en == GPIO_PULLUP_ENABLE),
            .intr_priority = 0,
            .log_level = config.log_level,
        }) {
    if (config.auto_init) {
      std::error_code ec;
      init(ec);
    }
    (void)config_.isr_core_id;
  }

  ~I2c() {
    std::error_code ec;
    deinit(ec);
  }

  void init(std::error_code &ec) {
    if (initialized_) {
      ec = std::make_error_code(std::errc::already_connected);
      return;
    }
    if (!bus_.init(ec)) {
      return;
    }
    initialized_ = true;
    ec.clear();
  }

  void deinit(std::error_code &ec) {
    devices_.clear();
    if (!bus_.deinit(ec)) {
      return;
    }
    initialized_ = false;
    ec.clear();
  }

  bool write(const uint8_t dev_addr, const uint8_t *data, const size_t data_len) {
    auto dev = get_device(dev_addr);
    if (!dev) {
      return false;
    }
    std::error_code ec;
    return dev->write(data, data_len, ec);
  }

  bool read(const uint8_t dev_addr, uint8_t *data, size_t data_len) {
    auto dev = get_device(dev_addr);
    if (!dev) {
      return false;
    }
    std::error_code ec;
    return dev->read(data, data_len, ec);
  }

  bool write_read(const uint8_t dev_addr, const uint8_t *write_data, const size_t write_size,
                  uint8_t *read_data, size_t read_size) {
    auto dev = get_device(dev_addr);
    if (!dev) {
      return false;
    }
    std::error_code ec;
    return dev->write_read(write_data, write_size, read_data, read_size, ec);
  }

  bool read_at_register(const uint8_t dev_addr, const uint8_t reg_addr, uint8_t *data,
                        size_t data_len) {
    auto dev = get_device(dev_addr);
    if (!dev) {
      return false;
    }
    std::error_code ec;
    return dev->read_register(reg_addr, data, data_len, ec);
  }

  bool probe_device(const uint8_t dev_addr) {
    std::error_code ec;
    if (!ensure_initialized(ec)) {
      return false;
    }
    return bus_.probe(dev_addr, static_cast<int32_t>(config_.timeout_ms), ec);
  }

  i2c_master_bus_handle_t handle() const { return bus_.handle(); }

  bool is_initialized() const { return initialized_; }

private:
  bool ensure_initialized(std::error_code &ec) {
    if (initialized_) {
      ec.clear();
      return true;
    }
    init(ec);
    return !ec;
  }

  std::shared_ptr<I2cMasterDevice<uint8_t>> get_device(uint8_t dev_addr) {
    std::error_code ec;
    if (!ensure_initialized(ec)) {
      return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(dev_addr);
    if (it != devices_.end()) {
      return it->second;
    }

    auto dev = bus_.add_device<uint8_t>(
        I2cMasterDevice<uint8_t>::Config{
            .device_address = dev_addr,
            .timeout_ms = static_cast<int>(config_.timeout_ms),
            .scl_speed_hz = config_.clk_speed,
            .auto_init = true,
            .log_level = config_.log_level,
        },
        ec);
    if (ec || !dev) {
      return nullptr;
    }
    devices_.emplace(dev_addr, dev);
    return dev;
  }

  Config config_;
  I2cMasterBus bus_;
  bool initialized_ = false;
  std::mutex mutex_;
  std::unordered_map<uint8_t, std::shared_ptr<I2cMasterDevice<uint8_t>>> devices_;
};

} // namespace espp
