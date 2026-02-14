#pragma once

#include <cstdint>
#include <memory>

namespace common {
class Mutex {
   public:
    Mutex();
    ~Mutex();

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    Mutex(Mutex&&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    void lock();
    void unlock();
    bool tryLock(const uint32_t timeoutMs = 0);
    bool isValid() const;

   private:
    struct Impl;
    std::unique_ptr<Impl> m;
};

}  // namespace common
