#pragma once

#include <cstddef>
#include <cstdint>

namespace adapters {
class II2cBus {
   public:
    virtual ~II2cBus() = default;

    virtual bool init() = 0;
    virtual bool writeBytes(const uint8_t& deviceAddr, const uint8_t* data, const size_t& len,
                            const uint32_t& timeoutMs) = 0;
    virtual bool readBytes(const uint8_t& deviceAddr, uint8_t* data, const size_t& len,
                           const uint32_t& timeoutMs) = 0;
};

}  // namespace adapters
