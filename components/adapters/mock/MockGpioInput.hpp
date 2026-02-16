#pragma once

#include <gmock/gmock.h>

#include "IGpioInput.hpp"

namespace adapters {
class MockGpioInput : public IGpioInput {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(void, deinit, (), (override));
    MOCK_METHOD(int, getLevel, (const uint32_t), (override));
};

}  // namespace adapters
