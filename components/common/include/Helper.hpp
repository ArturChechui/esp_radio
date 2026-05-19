/**
 * @file Helper.hpp
 * @brief Common utility functions for time and unit conversions.
 *
 * This file contains lightweight helper functions used to bridge the gap
 * between standard units (like milliseconds) and platform-specific types
 * (like FreeRTOS ticks).
 */

#pragma once

#include <freertos/FreeRTOS.h>
#include <stdint.h>

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @brief Converts a millisecond duration to FreeRTOS ticks.
 * * This function calculates the equivalent number of system ticks for a given
 * time in milliseconds. It performs a "ceiling" division to ensure that the
 * resulting delay is at least as long as requested.
 * * @param ms The duration in milliseconds to convert.
 * @return The equivalent number of TickType_t ticks.
 * - Returns portMAX_DELAY if the input is UINT32_MAX.
 * - Returns 0 if the input is 0.
 * - Returns the calculated ticks (rounded up) otherwise.
 */
inline TickType_t toTicks(const uint32_t ms) {
    if (ms == UINT32_MAX) {
        return portMAX_DELAY;
    }
    if (ms == 0U) {
        return 0;
    }

    constexpr uint32_t tickMs = portTICK_PERIOD_MS;               // 10ms when HZ=100
    return static_cast<TickType_t>((ms + tickMs - 1U) / tickMs);  // ceil
}

}  // namespace common
