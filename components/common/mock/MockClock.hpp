#pragma once

#include <gmock/gmock.h>

#include "IClock.hpp"

namespace common {
class MockClock : public IClock {
   public:
    MOCK_METHOD(void, sleepMs, (const uint32_t), (override));
    MOCK_METHOD(uint64_t, nowMs, (), (const, override));
};
}  // namespace common
