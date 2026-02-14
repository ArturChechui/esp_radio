#pragma once

#include "ISignal.hpp"

namespace common {
class FakeSignal : public ISignal {
   public:
    FakeSignal() = default;
    ~FakeSignal() override = default;

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
