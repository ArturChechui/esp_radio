/**
 * @file Signal.hpp
 * @brief Concrete implementation of the ISignal interface using FreeRTOS semaphores.
 *
 * This file defines the Signal class, which provides a thread-safe signaling
 * mechanism used to unblock tasks or synchronize activities across different threads.
 */

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "ISignal.hpp"

namespace common {

/**
 * @class Signal
 * @brief A synchronization primitive wrapper for FreeRTOS binary semaphores.
 *
 * This implementation uses `StaticSemaphore_t` to avoid heap allocation
 * for the semaphore itself, ensuring better determinism and reliability
 * in memory-constrained environments.
 */
class Signal : public ISignal {
   public:
    /**
     * @brief Constructs a Signal and initializes the static binary semaphore.
     */
    Signal();

    /**
     * @brief Destroys the signal and cleans up the RTOS semaphore handle.
     */
    ~Signal() override;

    /**
     * @brief Blocks the calling task until the signal is given or a timeout occurs.
     * @param timeoutMs Maximum time to wait in milliseconds.
     * @return true if the signal was received, false on timeout.
     */
    bool wait(const uint32_t& timeoutMs) const override;

    /**
     * @brief Gives the signal, unblocking any task currently waiting on it.
     */
    void signal() override;

    /**
     * @brief Checks if the underlying semaphore handle was successfully created.
     * @return true if the signal is valid and ready for use.
     */
    bool isValid() const override;

    /**
     * @brief Resets the signal to an unsignaled state.
     */
    void reset() override;

    /** @name Non-copyable
     * Signals manage unique RTOS resources and cannot be copied.
     * @{ */
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
    /** @} */

   private:
    StaticSemaphore_t mStorage; /**< Memory storage for the static semaphore. */
    SemaphoreHandle_t mHandle;  /**< Handle used by FreeRTOS to manage the semaphore. */
};

}  // namespace common
