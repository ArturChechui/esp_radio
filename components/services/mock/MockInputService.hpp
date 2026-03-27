#pragma once

#include <gmock/gmock.h>

#include "IInputService.hpp"

namespace services {
class MockInputService : public IInputService {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(void, deinit, (), (override));
    MOCK_METHOD(void, setMode, (const bool night), (override));
};
}  // namespace services
