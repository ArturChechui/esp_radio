#include "Clock.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace adapters {
void Clock::sleepMs(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}
}  // namespace adapters
