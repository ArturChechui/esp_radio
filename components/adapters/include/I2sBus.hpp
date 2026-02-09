#pragma once

#include <driver/i2s_std.h>

#include "II2sBus.hpp"

namespace adapters {
class I2sBus final : public II2sBus {
   public:
    I2sBus();
    ~I2sBus() override;

    bool init() override;
    void deinit() override;
    size_t write(const int16_t* data, const size_t& size, const uint32_t& timeoutMs) override;
    bool reconfigureClock(const uint32_t& sampleRate) override;
    uint32_t getSampleRate() const override;

   private:
    i2s_chan_handle_t mI2sTxHandle;
    uint32_t mCurrentSampleRate;
};

}  // namespace adapters
