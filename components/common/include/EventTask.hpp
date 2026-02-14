#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "IEventQueue.hpp"

namespace common {
class IEventHandler;
class ITaskRunner;
class IStopToken;
struct StepResult;

class EventTask : public IEventQueue {
   public:
    EventTask(const char* taskName, ITaskRunner& taskRunner);
    ~EventTask() override;

    bool init(IEventHandler& handler);
    bool post(const AppEvent& event) override;

   private:
    static StepResult processStepFn(void* arg, IStopToken& token);
    StepResult processStep(IStopToken& token);

    TaskParams mTaskParams;
    TaskHandle mTaskHandle;
    ITaskRunner& mTaskRunner;

    // IMPORTANT:
    // FreeRTOS queues copy elements with memcpy. AppEvent is a std::variant and
    // currently contains non-trivial types (e.g. std::string via WifiStateChangedEvent).
    // Copying such objects with memcpy causes memory corruption and random crashes.
    //
    // To make this safe, we pass pointers through the queue and allocate events on the heap.
    QueueHandle_t mEventQueue;
    bool mIsRunning;

    // TODO: think about Observer pattern:
    // observers list  std::vector<std::function<void(const AppEvent&)>> mObservers;
    // subscribe / unsubscribe methods
    IEventHandler* mHandler;  // Non-owning pointer
};

}  // namespace common
