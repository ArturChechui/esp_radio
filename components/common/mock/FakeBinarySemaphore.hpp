#pragma once

#include "IBinarySemaphore.hpp"

namespace common {
class FakeBinarySemaphore : public IBinarySemaphore {
   public:
    FakeBinarySemaphore() = default;
    ~FakeBinarySemaphore() override = default;

    bool wait(const uint32_t& timeoutMs) const override {
        return true;
    }
    void signal() override {}
    bool isValid() const override {
        return true;
    }
    void reset() override {}
};

}  // namespace common
