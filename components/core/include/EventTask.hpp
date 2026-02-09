#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "IEventQueue.hpp"

namespace common {
class IEventHandler;
}  // namespace common

namespace core {
/**
 * @brief Generic event-driven task
 * Implements IEventQueue so services can post events
 */
class EventTask : public common::IEventQueue {
   public:
    EventTask(const char* name, const uint8_t& queueSize, const uint16_t& stackSize,
              const uint8_t& priority);
    ~EventTask() override;

    /**
     * @brief Initialize and start the task
     */
    bool init(common::IEventHandler& handler);

    /**
     * @brief Post an event to the task queue (from IEventQueue)
     */
    bool post(const common::AppEvent& event) override;

   private:
    void runLoop();
    void stop();
    static void taskEntry(void* pvParameters);

    const char* mName;
    uint8_t mQueueSize;
    uint16_t mStackSize;
    uint8_t mPriority;
    QueueHandle_t mEventQueue;
    TaskHandle_t mTaskHandle;
    bool mIsRunning;
    // TODO: think about Observer pattern:
    // observers list  std::vector<std::function<void(const common::AppEvent&)>> mObservers;
    // subscribe / unsubscribe methods
    common::IEventHandler* mHandler;  // Non-owning pointer
    // TODO: use common for tasks and signal
    SemaphoreHandle_t mStoppedSem = nullptr;
};

}  // namespace core
