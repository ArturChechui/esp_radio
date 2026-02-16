#include "Clock.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace common {
void Clock::sleepMs(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

uint64_t Clock::nowMs() const {
    return static_cast<uint64_t>(pdTICKS_TO_MS(xTaskGetTickCount()));
}
}  // namespace common
