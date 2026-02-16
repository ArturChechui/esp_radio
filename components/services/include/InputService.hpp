#pragma once

#include <array>
#include <cstdint>

#include "Types.hpp"

namespace adapters {
class IGpioInput;
}  // namespace adapters

namespace common {
class IEventQueue;
template <typename T>
class IQueue;
class ITaskRunner;
class IClock;
}  // namespace common

namespace services {
class InputService {
   public:
    explicit InputService(adapters::IGpioInput& gpioInput, common::IEventQueue& coreEventQueue,
                          common::IQueue<uint32_t>& queue, common::ITaskRunner& runner,
                          common::IClock& clock);
    ~InputService() = default;

    bool init();
    void deinit();

   private:
    static common::StepResult processStepFn(void* arg, common::IStopToken& token);
    common::StepResult processStep(common::IStopToken& token);

    void handleEncoderGpio();
    void applyDetentsToVolume(const int detents);

    void handleButtonGpio(const uint32_t gpioNum, common::IStopToken& token);

    adapters::IGpioInput& mGpioInput;
    common::IEventQueue& mCoreEventQueue;
    common::IQueue<uint32_t>& mQueue;
    common::ITaskRunner& mTaskRunner;
    common::TaskHandle mTaskHandle;
    common::IClock& mClock;
    std::array<uint64_t, 3> mLastPressMs;

    uint8_t mEncPrevQuadratureState;
    int mEncQuarterAccumulator;
    bool mEncInvert;
    int mVolume;
};

}  // namespace services
