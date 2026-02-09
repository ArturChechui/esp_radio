#include "BinarySemaphore.hpp"

#include <freertos/FreeRTOS.h>

#include "Helper.hpp"

namespace common {
BinarySemaphore::BinarySemaphore() : mStorage(), mHandle(xSemaphoreCreateBinaryStatic(&mStorage)) {}

BinarySemaphore::~BinarySemaphore() {
    mHandle = nullptr;
}

bool BinarySemaphore::wait(const uint32_t& timeoutMs) const {
    if (mHandle == nullptr) {
        return false;
    }

    const auto res = xSemaphoreTake(mHandle, toTicks(timeoutMs));

    return (res == pdTRUE);
}

void BinarySemaphore::signal() {
    if (mHandle == nullptr) {
        return;
    }

    xSemaphoreGive(mHandle);
}

bool BinarySemaphore::isValid() const {
    return (mHandle != nullptr);
}

void BinarySemaphore::reset() {
    if (mHandle == nullptr) {
        return;
    }

    xSemaphoreTake(mHandle, 0);
}

}  // namespace common
