
#pragma once

#include <cstdint>

namespace common {
class IClock {
   public:
    virtual ~IClock() = default;
    virtual void sleepMs(const uint32_t ms) = 0;
    virtual uint64_t nowMs() const = 0;
};
}  // namespace common