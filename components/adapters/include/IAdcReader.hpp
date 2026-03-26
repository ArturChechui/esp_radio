#pragma once

#include <cstddef>
#include <cstdint>

namespace adapters {
class IAdcReader {
   public:
    static constexpr uint16_t MaxAdcMv = 3300U;
    static constexpr uint16_t MaxAdcRaw = 4095U;

    virtual ~IAdcReader() = default;

    virtual bool init() = 0;
    virtual bool setupChannel(const uint32_t gpioNum) = 0;
    virtual bool readRawBurst(const uint32_t gpioNum, int* buffer, const size_t count) = 0;
};
}  // namespace adapters
