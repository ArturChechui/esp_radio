#pragma once

#include <cstdint>

namespace common {
template <typename T>
class IQueue {
   public:
    virtual ~IQueue() = default;
    virtual bool push(const T& item, const uint32_t timeoutTicks = 100) = 0;
    virtual bool get(T& out) = 0;
    virtual bool tryGet(T& out) = 0;
};
}  // namespace common
