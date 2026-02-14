#include "Mutex.hpp"

namespace common {
struct Mutex::Impl {};

Mutex::Mutex() : m(std::make_unique<Impl>()) {}

Mutex::~Mutex() = default;

void Mutex::lock() {}

void Mutex::unlock() {}

bool Mutex::tryLock(const uint32_t timeoutMs) {
    return true;
}

bool Mutex::isValid() const {
    return true;
}

}  // namespace common
