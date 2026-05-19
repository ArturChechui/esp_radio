/**
 * @file ISignal.hpp
 * @brief Interface for a thread synchronization signal.
 *
 * This file defines the ISignal interface, providing an abstraction for
 * basic signaling between tasks, similar to a binary semaphore or event flag.
 */

#pragma once

#include <cstdint>

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class ISignal
 * @brief Abstract interface for a synchronization event or signal.
 *
 * Implementations of this interface allow one task to wait for a notification
 * from another task or an Interrupt Service Routine (ISR).
 */
class ISignal {
   public:
    /** @brief Virtual destructor for safe interface cleanup. */
    virtual ~ISignal() = default;

    /**
     * @brief Blocks the calling task until the signal is received or a timeout occurs.
     * @param timeoutMs Maximum time to wait in milliseconds.
     * @return true if the signal was received, false if the timeout expired.
     */
    virtual bool wait(const uint32_t& timeoutMs) const = 0;

    /**
     * @brief Sets the signal, unblocking any task currently waiting on it.
     * * If no task is waiting, the next call to wait() may return immediately
     * depending on the specific implementation (e.g., binary semaphore behavior).
     */
    virtual void signal() = 0;

    /**
     * @brief Checks if the underlying synchronization primitive is initialized and valid.
     * @return true if the signal is ready for use.
     */
    virtual bool isValid() const = 0;

    /**
     * @brief Resets the signal to its unsignaled state.
     */
    virtual void reset() = 0;
};

}  // namespace common
