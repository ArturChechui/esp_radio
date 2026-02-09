#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "IBinarySemaphore.hpp"

namespace common {
class BinarySemaphore : public IBinarySemaphore {
   public:
    BinarySemaphore();
    ~BinarySemaphore() override;

    bool wait(const uint32_t& timeoutMs) const override;
    void signal() override;
    bool isValid() const override;
    void reset() override;

    // Prevent copying
    BinarySemaphore(const BinarySemaphore&) = delete;
    BinarySemaphore& operator=(const BinarySemaphore&) = delete;

   private:
    StaticSemaphore_t mStorage;
    SemaphoreHandle_t mHandle;
};

}  // namespace common
