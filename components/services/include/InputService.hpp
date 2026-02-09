#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "Types.hpp"

namespace adapters {
class IGpioInput;
}  // namespace adapters

namespace common {
class IEventQueue;
}  // namespace common

namespace services {
class InputService {
   public:
    explicit InputService(adapters::IGpioInput& gpioInput, common::IEventQueue& coreEventQueue);
    ~InputService() = default;

    bool init();
    void deinit();

   private:
    void onGpioInputData(const common::GpioInputData& data);

    adapters::IGpioInput& mGpioInput;
    common::IEventQueue& mCoreEventQueue;
};

}  // namespace services
