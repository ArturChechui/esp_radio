
#pragma once

#include <cstdint>

#include "IClock.hpp"

namespace common {
class Clock : public IClock {
   public:
    Clock() = default;
    ~Clock() override = default;

    void sleepMs(const uint32_t ms) override;
    uint64_t nowMs() const override;
};
}  // namespace common
