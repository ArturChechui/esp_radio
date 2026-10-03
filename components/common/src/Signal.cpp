#include "Signal.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "Helper.hpp"

namespace common {
struct Signal::Impl {
    StaticSemaphore_t storage; /**< Memory storage for the static semaphore. */
    SemaphoreHandle_t handle;  /**< Handle used by FreeRTOS to manage the semaphore. */

    Impl() {
        handle = xSemaphoreCreateBinaryStatic(&storage);
    }
};

Signal::Signal() : m(std::make_unique<Impl>()) {}

Signal::~Signal() = default;

bool Signal::wait(const uint32_t& timeoutMs) const {
    if (m->handle == nullptr) {
        return false;
    }

    const auto res = xSemaphoreTake(m->handle, toTicks(timeoutMs));

    return (res == pdTRUE);
}

void Signal::signal() {
    if (m->handle == nullptr) {
        return;
    }

    xSemaphoreGive(m->handle);
}

bool Signal::isValid() const {
    return (m->handle != nullptr);
}

void Signal::reset() {
    if (m->handle == nullptr) {
        return;
    }

    xSemaphoreTake(m->handle, 0);
}

}  // namespace common
