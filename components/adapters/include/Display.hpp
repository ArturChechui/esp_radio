#pragma once

#include <cstdint>
#include <vector>

#include "IDisplay.hpp"

namespace adapters {
class II2cBus;

class Display final : public IDisplay {
   public:
    explicit Display(II2cBus &i2cBus);
    ~Display() override;

    // IDisplay
    bool init() override;
    bool showFramebuffer(const uint8_t *framebuffer, const size_t &len) override;

   private:
    bool writeCommand(const uint8_t *cmd, const uint16_t &len);
    bool writeData(const uint8_t *data, const uint16_t &len);

    II2cBus &mI2cBus;
    uint8_t mI2cAddr;
    bool mReady;
};

}  // namespace adapters
