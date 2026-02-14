#include "Mutex.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace common {
struct Mutex::Impl {
    StaticSemaphore_t storage{};
    SemaphoreHandle_t handle{nullptr};

    Impl() {
        handle = xSemaphoreCreateMutexStatic(&storage);
    }
};

Mutex::Mutex() : m(std::make_unique<Impl>()) {}

Mutex::~Mutex() = default;

void Mutex::lock() {
    (void)xSemaphoreTake(m->handle, portMAX_DELAY);
}

void Mutex::unlock() {
    (void)xSemaphoreGive(m->handle);
}

bool Mutex::tryLock(uint32_t timeoutMs) {
    const TickType_t ticks = (timeoutMs == 0xFFFFFFFF) ? portMAX_DELAY : pdMS_TO_TICKS(timeoutMs);

    return (xSemaphoreTake(m->handle, ticks) == pdTRUE);
}

bool Mutex::isValid() const {
    return (m->handle != nullptr);
}

}  // namespace common
