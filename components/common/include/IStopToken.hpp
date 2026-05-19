/**
 * @file IStopToken.hpp
 * @brief Interface for task cancellation and interruptible sleeps.
 *
 * This file defines the IStopToken interface, which allows long-running
 * background tasks to check for stop requests and perform delays that
 * can be cut short if the system needs to shut down.
 */

#pragma once

#include <cstdint>

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class IStopToken
 * @brief Abstract interface for monitoring task cancellation.
 *
 * Objects implementing this interface are typically passed to worker
 * loops (e.g., in EventTask or AudioPlayer) to ensure they can be
 * stopped gracefully by the system manager.
 */
class IStopToken {
   public:
    /** @brief Virtual destructor for safe interface cleanup. */
    virtual ~IStopToken() = default;

    /**
     * @brief Checks if the owner of the task has requested a stop.
     * @return true if the task should terminate its execution loop.
     */
    virtual bool stopRequested() const = 0;

    /**
     * @brief Performs a delay that exits early if a stop is requested.
     * * This is more efficient than a standard sleep because it allows
     * the task to respond to shutdown signals immediately rather than
     * waiting for the full timer to expire.
     * * @param ms The maximum duration to sleep in milliseconds.
     * @return true if the full sleep duration elapsed, false if interrupted by a stop request.
     */
    virtual bool sleepMs(const uint32_t ms) = 0;
};

}  // namespace common
