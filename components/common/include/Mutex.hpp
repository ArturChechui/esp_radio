#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace common {

/**
 * @brief Thread-safe mutex wrapper (FreeRTOS semaphore)
 *
 * Handles creation/deletion, safe for concurrent access.
 * Use with SemaphoreGuard for automatic locking/unlocking.
 */
class Mutex {
   public:
    /**
     * @brief Create a new mutex
     */
    Mutex();

    /**
     * @brief Destroy mutex and free resources
     */
    ~Mutex();

    /**
     * @brief Get native FreeRTOS handle (for SemaphoreGuard)
     */
    SemaphoreHandle_t handle() const;

    /**
     * @brief Check if mutex is valid
     */
    bool isValid() const;

    // Prevent copying
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    // Allow moving
    Mutex(Mutex&& other) noexcept;
    Mutex& operator=(Mutex&& other) noexcept;

   private:
    // Static storage for the mutex
    StaticSemaphore_t mStorage{};
    SemaphoreHandle_t mHandle;
};

}  // namespace common
