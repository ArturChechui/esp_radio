
#pragma once

#include <cstdint>

namespace adapters {
class IClock {
   public:
    virtual ~IClock() = default;
    virtual void sleepMs(uint32_t ms) = 0;
};
}  // namespace adapters