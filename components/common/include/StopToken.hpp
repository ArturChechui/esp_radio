#pragma once

#include <cstdint>

#include "IStopToken.hpp"
#include "Types.hpp"

namespace common {
class TaskRunner;

class StopToken : public IStopToken {
   public:
    bool stopRequested() const override;
    bool sleepMs(const uint32_t ms) override;

   private:
    friend class TaskRunner;
    StopToken(TaskRunner& r, const TaskHandle& h);
    ~StopToken() override = default;

    TaskRunner& mRunner;
    TaskHandle mHandle;
};

}  // namespace common
