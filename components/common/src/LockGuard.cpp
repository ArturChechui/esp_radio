#include "LockGuard.hpp"

#include "Mutex.hpp"

namespace common {
LockGuard::LockGuard(Mutex& m, const uint32_t timeoutMs)
    : mMutex(&m), mLocked(m.tryLock(timeoutMs)) {}

LockGuard::~LockGuard() {
    if (mMutex && mLocked) {
        mMutex->unlock();
    }
}

}  // namespace common
