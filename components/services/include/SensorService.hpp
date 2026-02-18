#pragma once

#include <array>
#include <cstdint>

#include "Types.hpp"

namespace adapters {
class II2cBus;
}  // namespace adapters

namespace common {
class IEventQueue;
class ITaskRunner;
}  // namespace common

namespace services {
class SensorService {
   public:
    explicit SensorService(adapters::II2cBus& i2cBus, common::IEventQueue& coreEventQueue,
                           common::ITaskRunner& runner);
    ~SensorService();

    bool init();
    void deinit();

   private:
    static common::StepResult readStepFn(void* arg, common::IStopToken& token);
    common::StepResult readStep(common::IStopToken& token);
    bool aht20Parse(const std::array<uint8_t, 6>& data, int8_t& temperatureC, uint8_t& humidityPct);
    bool initAht20(common::IStopToken& token);

    adapters::II2cBus& mI2cBus;
    common::IEventQueue& mCoreEventQueue;
    common::ITaskRunner& mTaskRunner;
    common::TaskHandle mTaskHandle;

    bool mIsAht20Inited;
};

}  // namespace services
