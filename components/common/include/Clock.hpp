/**
 * @file Clock.hpp
 * @brief Concrete implementation of the IClock interface.
 *
 * This file contains the Clock class, which provides standard timing
 * and delay utilities used throughout the application.
 */

#pragma once

#include <cstdint>

#include "IClock.hpp"

namespace common {

/**
 * @class Clock
 * @brief System clock implementation for time tracking and thread sleeping.
 *
 * This class provides a wrapper around the system's time-keeping mechanisms,
 * allowing components to perform millisecond-accurate delays and retrieve
 * the current system uptime.
 */
class Clock : public IClock {
   public:
    /** @brief Default constructor. */
    Clock() = default;

    /** @brief Default virtual destructor. */
    ~Clock() override = default;

    /**
     * @brief Suspends the execution of the calling thread for a specified duration.
     * @param ms Duration to sleep in milliseconds.
     */
    void sleepMs(const uint32_t ms) override;

    /**
     * @brief Retrieves the current system uptime in milliseconds.
     * @return Total milliseconds elapsed since the system started.
     */
    uint64_t nowMs() const override;
};

}  // namespace common
