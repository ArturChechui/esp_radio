#include "StopToken.hpp"

#include "TaskRunner.hpp"
#include "Types.hpp"

namespace common {
StopToken::StopToken(TaskRunner& r, const TaskHandle& h) : mRunner(r), mHandle(h) {}

bool StopToken::stopRequested() const {
    return mRunner.isStopRequested(mHandle);
}

bool StopToken::sleepMs(uint32_t ms) {
    return mRunner.interruptibleSleep(mHandle, ms);
}

}  // namespace common
