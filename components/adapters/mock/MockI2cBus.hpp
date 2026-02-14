#pragma once

#include <gmock/gmock.h>

#include "II2cBus.hpp"

namespace adapters {
class MockI2cBus : public II2cBus {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(bool, writeBytes, (const uint8_t, const uint8_t*, const size_t, const uint32_t),
                (override));
    MOCK_METHOD(bool, readBytes, (const uint8_t, uint8_t*, const size_t, const uint32_t),
                (override));
};

}  // namespace adapters
