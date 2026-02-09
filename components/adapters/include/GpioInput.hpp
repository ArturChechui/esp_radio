#pragma once

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <array>

#include "IGpioInput.hpp"
#include "Mutex.hpp"

namespace common {
class Mutex;
}  // namespace common

namespace adapters {

/**
 * @brief ESP32 GPIO input adapter
 * Handles button presses via GPIO interrupts with debouncing
 */
class GpioInput : public IGpioInput {
   public:
    GpioInput();
    ~GpioInput() override;

    bool init() override;
    void deinit() override;
    void setInputCallback(common::GpioInputDataCallback cb) override;

   private:
    static void gpioIsrHandler(void* arg);
    static void gpioTaskFn(void* arg);
    void gpioTaskLoop();

    void dispatchButtonPress(const uint32_t gpioNum);

    static GpioInput* sInstance;  // Static instance for ISR access

    common::GpioInputDataCallback mInputDataCb;
    common::Mutex mCallbackMutex;
    bool mIsInitialized;

    QueueHandle_t mIsrQueue;
    TaskHandle_t mTaskHandle;

    std::array<TickType_t, 3> mLastPressTick;
};

}  // namespace adapters
