#include "SemaphoreGuard.hpp"

#include "Mutex.hpp"

namespace common {
SemaphoreGuard::SemaphoreGuard(SemaphoreHandle_t m, const TickType_t& timeout)
    : mMutex(m), mIsLocked(false) {
    if (mMutex) {
        mIsLocked = (xSemaphoreTake(mMutex, timeout) == pdTRUE);
    }
}

SemaphoreGuard::SemaphoreGuard(const Mutex& m, const TickType_t& timeout)
    : mMutex(m.handle()), mIsLocked(false) {
    if (mMutex) {
        mIsLocked = (xSemaphoreTake(mMutex, timeout) == pdTRUE);
    }
}

SemaphoreGuard::~SemaphoreGuard() {
    if (mMutex && mIsLocked) {
        xSemaphoreGive(mMutex);
    }
}

bool SemaphoreGuard::isLocked() const {
    return mIsLocked;
}

}  // namespace common
