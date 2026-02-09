#pragma once

#include <freertos/FreeRTOS.h>
#include <stdint.h>

namespace common {
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
