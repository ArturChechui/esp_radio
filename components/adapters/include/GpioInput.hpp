#pragma once

#include <cstdint>

#include "IGpioInput.hpp"

namespace common {
template <typename T>
class Queue;
}  // namespace common

namespace adapters {
class GpioInput : public IGpioInput {
   public:
    GpioInput(common::Queue<uint32_t>& queue);
    ~GpioInput() override;

    bool init() override;
    void deinit() override;
    int getLevel(const uint32_t gpioNum) override;

   private:
    static void gpioIsrHandler(void* arg);

    static GpioInput* sInstance;  // Static instance for ISR access

    common::Queue<uint32_t>& mQueue;
    bool mIsInitialized;
};
}  // namespace adapters
