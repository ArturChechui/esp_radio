#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <atomic>
#include <cstdint>
#include <cstring>

#include "ITaskRunner.hpp"
#include "Signal.hpp"
#include "Types.hpp"

namespace common {
class StopToken;

class TaskRunner : public ITaskRunner {
   public:
    static constexpr size_t MaxTasks = 8UL;

    TaskRunner();
    ~TaskRunner() override = default;

    TaskRunner(const TaskRunner&) = delete;
    TaskRunner& operator=(const TaskRunner&) = delete;

    TaskHandle start(const TaskParams& params, uint32_t stackWords, StepFn fn, void* user) override;
    StopResult stop(const TaskHandle& h, uint32_t waitMs) override;

   private:
    struct Slot {
        TaskRunner* owner{nullptr};

        std::atomic<bool> inUse{false};
        std::atomic<bool> stopRequested{false};
        std::atomic<uint16_t> runId{0U};

        TaskHandle_t task{nullptr};
        common::Signal done{};

        StepFn fn{nullptr};
        void* user{nullptr};

        // Internal storage (allocated by runner)
        StaticTask_t* tcb{nullptr};
        StackType_t* stack{nullptr};
        uint32_t stackWords{0U};

        char name[configMAX_TASK_NAME_LEN]{};
        uint16_t index{0U};
    };

    // internal helpers
    int findFreeSlot();
    bool validateHandle(const TaskHandle& h) const;
    bool isStopRequested(const TaskHandle& h) const;
    bool interruptibleSleep(const TaskHandle& h, uint32_t ms);
    void cleanupSlotFromTask(Slot& s);

    static void taskEntry(void* arg);

    friend class StopToken;

    Slot mSlots[MaxTasks];
};

}  // namespace common
