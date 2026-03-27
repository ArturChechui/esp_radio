#pragma once

#include <gmock/gmock.h>

#include "ISensorService.hpp"

namespace services {
class MockSensorService : public ISensorService {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(void, deinit, (), (override));
    MOCK_METHOD(void, setPlaybackActive, (const bool active), (override));
};
}  // namespace services
