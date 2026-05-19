/**
 * @file IClock.hpp
 * @brief Interface for system clock and timing services.
 *
 * This file defines the IClock interface, which provides abstract methods
 * for thread sleeping and retrieving the system's current timestamp.
 */

#pragma once

#include <cstdint>

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class IClock
 * @brief Abstract interface for time-keeping and execution delays.
 *
 * Implementations of this interface allow components to perform
 * millisecond-accurate timing operations without being tied to a
 * specific OS or hardware timer implementation.
 */
class IClock {
   public:
    /** @brief Virtual destructor for safe interface cleanup. */
    virtual ~IClock() = default;

    /**
     * @brief Suspends the calling thread for a specific number of milliseconds.
     * @param ms The duration to sleep in milliseconds.
     */
    virtual void sleepMs(const uint32_t ms) = 0;

    /**
     * @brief Retrieves the current system uptime.
     * @return The number of milliseconds elapsed since the system started.
     */
    virtual uint64_t nowMs() const = 0;
};

}  // namespace common
