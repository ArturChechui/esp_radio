#pragma once

#include <cstdint>

namespace common {
class ISignal {
   public:
    virtual ~ISignal() = default;

    virtual bool wait(const uint32_t& timeoutMs) const = 0;
    virtual void signal() = 0;
    virtual bool isValid() const = 0;
    virtual void reset() = 0;
};

}  // namespace common
