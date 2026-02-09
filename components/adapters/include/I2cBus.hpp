#pragma once

#include <driver/i2c_master.h>

#include <map>

#include "II2cBus.hpp"

namespace adapters {
class I2cBus final : public II2cBus {
   public:
    I2cBus();
    ~I2cBus() override;

    bool init() override;
    bool writeBytes(const uint8_t& deviceAddr, const uint8_t* data, const size_t& len,
                    const uint32_t& timeoutMs) override;
    bool readBytes(const uint8_t& deviceAddr, uint8_t* data, const size_t& len,
                   const uint32_t& timeoutMs) override;

   private:
    i2c_master_dev_handle_t getOrCreateDeviceHandle(const uint8_t& deviceAddr);

    int mPort;
    i2c_master_bus_handle_t mBusHandle;
    uint32_t mFreqHz;
    std::map<uint8_t, i2c_master_dev_handle_t> mDeviceHandles;
};

}  // namespace adapters
