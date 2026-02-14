#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "ISignal.hpp"

namespace common {
class Signal : public ISignal {
   public:
    Signal();
    ~Signal() override;

    bool wait(const uint32_t& timeoutMs) const override;
    void signal() override;
    bool isValid() const override;
    void reset() override;

    // Prevent copying
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

   private:
    StaticSemaphore_t mStorage;
    SemaphoreHandle_t mHandle;
};

}  // namespace common
