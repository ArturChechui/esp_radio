

#pragma once

#include <gmock/gmock.h>

#include "ITaskRunner.hpp"
#include "Types.hpp"

namespace common {
class MockTaskRunner : public ITaskRunner {
   public:
    MOCK_METHOD(TaskHandle, start,
                (const TaskParams& params, uint32_t stackWords, StepFn fn, void* user), (override));
    MOCK_METHOD(StopResult, stop, (const TaskHandle& h, uint32_t waitMs), (override));
};

}  // namespace common
