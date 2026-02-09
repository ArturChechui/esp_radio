

#pragma once

#include <gmock/gmock.h>

#include "IEventQueue.hpp"

namespace common {
class MockEventQueue : public IEventQueue {
   public:
    MOCK_METHOD(bool, post, (const AppEvent&), (override));
};

}  // namespace common
