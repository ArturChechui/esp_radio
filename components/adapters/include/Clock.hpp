
#pragma once

#include <cstdint>

#include "IClock.hpp"

namespace adapters {
class Clock : public IClock {
   public:
    Clock() = default;

    ~Clock() override = default;
    void sleepMs(uint32_t ms) override;
};
}  // namespace adapters
