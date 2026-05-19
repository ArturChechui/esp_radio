/**
 * @file TaskRunner.hpp
 * @brief Concrete implementation of the ITaskRunner interface.
 *
 * This file defines the TaskRunner class, which manages a fixed-size pool
 * of task slots. It handles static memory allocation for task stacks and
 * TCBs (Task Control Blocks) to ensure high reliability.
 */

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <atomic>
#include <cstdint>
#include <cstring>

#include "ITaskRunner.hpp"
#include "Signal.hpp"
#include "Types.hpp"

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

class StopToken; /**< Forward declaration of the StopToken class. */

/**
 * @class TaskRunner
 * @brief A task manager providing lifecycle control and static allocation.
 *
 * The TaskRunner allows the application to start and stop background tasks
 * while maintaining a registry of active tasks. It uses a "slot" system
 * to reuse memory and provides synchronization mechanisms to ensure
 * tasks have fully exited before their resources are reused.
 */
class TaskRunner : public ITaskRunner {
   public:
    /** @brief Maximum number of concurrent tasks this runner can manage. */
    static constexpr size_t MaxTasks = 8UL;

    /**
     * @brief Constructs the TaskRunner and initializes the internal slot registry.
     */
    TaskRunner();

    /** @brief Virtual destructor. */
    ~TaskRunner() override = default;

    /** @name Non-copyable
     * The TaskRunner manages hardware-bound task resources and cannot be copied.
     * @{ */
    TaskRunner(const TaskRunner&) = delete;
    TaskRunner& operator=(const TaskRunner&) = delete;
    /** @} */

    /**
     * @brief Creates and starts a new task in an available slot.
     * * This implementation performs static allocation for the task stack and
     * TCB. It wraps the user function in an internal logic that handles
     * completion signaling.
     * * @param params Parameters including the task name and priority.
     * @param stackWords The required stack size in words.
     * @param fn The user-defined function to run.
     * @param user Context pointer passed to the user function.
     * @return A TaskHandle containing the slot index and run ID, or an invalid handle on failure.
     */
    TaskHandle start(const TaskParams& params, uint32_t stackWords, StepFn fn, void* user) override;

    /**
     * @brief Signals a task to stop and waits for it to clean up.
     * @param h The handle of the task to stop.
     * @param waitMs Maximum time to wait for the task to signal completion.
     * @return StopResult::Ok if the task stopped, or an error code indicating a timeout or invalid
     * handle.
     */
    StopResult stop(const TaskHandle& h, uint32_t waitMs) override;

   private:
    /**
     * @struct Slot
     * @brief Internal container for task-specific state and memory.
     */
    struct Slot {
        TaskRunner* owner{nullptr}; /**< Pointer back to the parent runner. */

        std::atomic<bool> inUse{false}; /**< Indicates if this slot is currently occupied. */
        std::atomic<bool> stopRequested{
            false};                      /**< Flag monitored by StopTokens for cancellation. */
        std::atomic<uint16_t> runId{0U}; /**< Incrementing ID to prevent stale handle usage. */

        TaskHandle_t task{nullptr}; /**< The FreeRTOS task handle. */
        common::Signal done{};      /**< Signal triggered when the task function exits. */

        StepFn fn{nullptr};  /**< The user function to execute. */
        void* user{nullptr}; /**< User context pointer. */

        // Internal storage (allocated by runner)
        StaticTask_t* tcb{nullptr};  /**< Static TCB storage. */
        StackType_t* stack{nullptr}; /**< Static stack storage. */
        uint32_t stackWords{0U};     /**< Current stack size in words. */

        char name[configMAX_TASK_NAME_LEN]{}; /**< Task name for OS monitoring. */
        uint16_t index{0U};                   /**< The index of this slot in the array. */
    };

    /** @name Internal Helpers
     * @{ */

    /** @brief Scans the registry for an unused Slot. */
    int findFreeSlot();

    /** @brief Validates that a handle belongs to an active task in this runner. */
    bool validateHandle(const TaskHandle& h) const;

    /** @brief Checks if a specific task has been requested to terminate. */
    bool isStopRequested(const TaskHandle& h) const;

    /** @brief Provides interruptible delay functionality for StopTokens. */
    bool interruptibleSleep(const TaskHandle& h, const uint32_t ms);

    /** @brief Removes all data from slot for correct task shutdown */
    void cleanupSlotFromTask(Slot& s);

    /** @brief Static entry point passed to xTaskCreateStatic. */
    static void taskEntry(void* arg);
    /** @} */

    friend class StopToken; /**< StopToken needs access to internal state. */
    Slot mSlots[MaxTasks];  /**< The fixed pool of task slots. */
};

}  // namespace common
