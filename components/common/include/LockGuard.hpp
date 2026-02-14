#pragma once

#include "Mutex.hpp"

namespace common {
class LockGuard {
   public:
    explicit LockGuard(Mutex& m, const uint32_t timeoutMs = 0xFFFFFFFF);
    ~LockGuard();

    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

   private:
    Mutex* mMutex;
    bool mLocked;
};

}  // namespace common
