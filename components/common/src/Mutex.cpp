#include "Mutex.hpp"

namespace common {
Mutex::Mutex() : mHandle(xSemaphoreCreateMutexStatic(&mStorage)) {}

Mutex::~Mutex() {
    mHandle = nullptr;
}

SemaphoreHandle_t Mutex::handle() const {
    return mHandle;
}

bool Mutex::isValid() const {
    return (mHandle != nullptr);
}

Mutex::Mutex(Mutex&& other) noexcept : mHandle(other.mHandle) {
    other.mHandle = nullptr;
}

Mutex& Mutex::operator=(Mutex&& other) noexcept {
    if (this != &other) {
        mHandle = other.mHandle;
        other.mHandle = nullptr;
    }

    return *this;
}

}  // namespace common
