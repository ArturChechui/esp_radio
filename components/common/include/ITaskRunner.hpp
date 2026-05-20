/**
 * @file ITaskRunner.hpp
 * @brief Interface for managing background tasks and thread lifecycles.
 *
 * This file defines the ITaskRunner interface, which provides an abstraction
 * for starting and stopping asynchronous worker threads or tasks.
 */

#pragma once

#include <cstdint>

#include "Types.hpp"

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class ITaskRunner
 * @brief Abstract interface for a task execution service.
 *
 * Implementations of this interface (e.g., a FreeRTOS-based runner)
 * handle the low-level details of stack allocation, task priority,
 * and OS-specific task creation.
 */
class ITaskRunner {
   public:
    /** @brief Virtual destructor for safe interface cleanup. */
    virtual ~ITaskRunner() = default;

    /**
     * @brief Creates and starts a new background task.
     * @param params Configuration parameters including task name and priority.
     * @param stackWords The size of the task stack, measured in words (typically 4 bytes on ESP32).
     * @param fn The function to be executed as the task's main loop.
     * @param user User-defined pointer passed as an argument to the task function.
     * @return A handle to the newly created task, or an empty handle on failure.
     */
    virtual TaskHandle start(const TaskParams& params, uint32_t stackWords, StepFn fn,
                             void* user) = 0;

    /**
     * @brief Requests a task to stop and waits for it to terminate.
     * @param h Handle to the task to be stopped.
     * @param waitMs Maximum time in milliseconds to wait for the task to finish.
     * @return StopResult indicating if the task stopped gracefully, timed out, or encountered an
     * error.
     */
    virtual StopResult stop(const TaskHandle& h, uint32_t waitMs) = 0;
};

}  // namespace common
