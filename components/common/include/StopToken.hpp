/**
 * @file StopToken.hpp
 * @brief Concrete implementation of the IStopToken interface.
 *
 * This file defines the StopToken class, which is used by worker tasks
 * to monitor their own execution state and respond to shutdown requests
 * from the TaskRunner.
 */

#pragma once

#include <cstdint>

#include "IStopToken.hpp"
#include "Types.hpp"

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

class TaskRunner; /**< Forward declaration of the TaskRunner class. */

/**
 * @class StopToken
 * @brief A task-specific token for managing execution lifecycle.
 *
 * This class is tightly coupled with the TaskRunner. It allows a task
 * to query whether it should stop and provides a mechanism for
 * interruptible delays.
 */
class StopToken : public IStopToken {
   public:
    /**
     * @brief Checks if the TaskRunner has requested this specific task to stop.
     * @return true if a stop request is pending for the associated TaskHandle.
     */
    bool stopRequested() const override;

    /**
     * @brief Performs a delay that exits early if a stop is requested.
     * @param ms The maximum duration to sleep in milliseconds.
     * @return true if the full sleep duration elapsed, false if interrupted.
     */
    bool sleepMs(const uint32_t ms) override;

   private:
    /** * @brief TaskRunner is the only component allowed to create StopTokens.
     */
    friend class TaskRunner;

    /**
     * @brief Constructs a StopToken for a specific task.
     * @param r Reference to the runner managing the task.
     * @param h The handle identifying the specific task instance.
     */
    StopToken(TaskRunner& r, const TaskHandle& h);

    /** @brief Destructor is private to ensure lifecycle is managed by TaskRunner. */
    ~StopToken() override = default;

    TaskRunner& mRunner; /**< Reference to the parent TaskRunner. */
    TaskHandle mHandle;  /**< The handle of the task using this token. */
};

}  // namespace common
