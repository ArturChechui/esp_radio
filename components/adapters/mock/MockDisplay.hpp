#pragma once

#include <gmock/gmock.h>

#include "IDisplay.hpp"

namespace adapters {
class MockDisplay : public IDisplay {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(bool, showFramebuffer, (const uint8_t*, const size_t), (override));
    MOCK_METHOD(bool, showWindow,
                (const uint8_t, const uint8_t, const uint8_t, const uint8_t, const uint8_t*,
                 const size_t),
                (override));
};

}  // namespace adapters
