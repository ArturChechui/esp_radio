#pragma once

#include <gmock/gmock.h>

#include "II2sBus.hpp"

namespace adapters {
class MockI2sBus : public II2sBus {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(void, deinit, (), (override));
    MOCK_METHOD(size_t, write, (const int16_t*, const size_t&, const uint32_t&), (override));
    MOCK_METHOD(bool, reconfigureClock, (const uint32_t&), (override));
    MOCK_METHOD(uint32_t, getSampleRate, (), (const, override));
};

}  // namespace adapters
