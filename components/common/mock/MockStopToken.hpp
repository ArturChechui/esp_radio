

#pragma once

#include <gmock/gmock.h>

#include "IStopToken.hpp"

namespace common {
class MockStopToken : public IStopToken {
   public:
    MOCK_METHOD(bool, stopRequested, (), (const, override));
    MOCK_METHOD(bool, sleepMs, (const uint32_t), (override));
};

}  // namespace common
