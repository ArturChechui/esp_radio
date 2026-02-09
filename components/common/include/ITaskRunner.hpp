#pragma once

#include <cstdint>

#include "Types.hpp"

namespace common {
class ITaskRunner {
   public:
    virtual ~ITaskRunner() = default;

    virtual TaskHandle start(const TaskParams& params, uint32_t stackWords, StepFn fn,
                             void* user) = 0;
    virtual StopResult stop(const TaskHandle& h, uint32_t waitMs) = 0;
};

}  // namespace common
