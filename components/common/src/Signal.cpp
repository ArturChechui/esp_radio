#include <freertos/FreeRTOS.h>

#include "Helper.hpp"
#include "Signal.hpp"

namespace common {
Signal::Signal() : mStorage(), mHandle(xSemaphoreCreateBinaryStatic(&mStorage)) {}

Signal::~Signal() {
    mHandle = nullptr;
}

bool Signal::wait(const uint32_t& timeoutMs) const {
    if (mHandle == nullptr) {
        return false;
    }

    const auto res = xSemaphoreTake(mHandle, toTicks(timeoutMs));

    return (res == pdTRUE);
}

void Signal::signal() {
    if (mHandle == nullptr) {
        return;
    }

    xSemaphoreGive(mHandle);
}

bool Signal::isValid() const {
    return (mHandle != nullptr);
}

void Signal::reset() {
    if (mHandle == nullptr) {
        return;
    }

    xSemaphoreTake(mHandle, 0);
}

}  // namespace common
