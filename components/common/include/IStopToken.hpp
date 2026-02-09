#pragma once

#include <cstdint>

namespace common {
class IStopToken {
   public:
    virtual ~IStopToken() = default;

    virtual bool stopRequested() const = 0;
    virtual bool sleepMs(const uint32_t ms) = 0;
};

}  // namespace common
