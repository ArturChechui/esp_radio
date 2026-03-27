#pragma once

#include <gmock/gmock.h>

#include "IAdcReader.hpp"

namespace adapters {
class MockAdcReader : public IAdcReader {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(bool, setupChannel, (const uint32_t gpioNum), (override));
    MOCK_METHOD(bool, readRawBurst,
                (const uint32_t gpioNum, int* buffer, const size_t count), (override));
};
}  // namespace adapters
