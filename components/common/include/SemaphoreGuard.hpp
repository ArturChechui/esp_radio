#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace common {
class Mutex;

class SemaphoreGuard {
   public:
    explicit SemaphoreGuard(SemaphoreHandle_t m, const TickType_t& timeout = portMAX_DELAY);
    explicit SemaphoreGuard(const Mutex& m, const TickType_t& timeout = portMAX_DELAY);
    ~SemaphoreGuard();

    // Prevent copying
    SemaphoreGuard(const SemaphoreGuard&) = delete;
    SemaphoreGuard& operator=(const SemaphoreGuard&) = delete;

    bool isLocked() const;

   private:
    SemaphoreHandle_t mMutex;
    bool mIsLocked;
};

}  // namespace common
