#pragma once

#include <cstddef>
#include <cstdint>

#include "IDisplay.hpp"
#include "II2cBus.hpp"

namespace adapters {

class Display final : public IDisplay {
   public:
    explicit Display(II2cBus& i2cBus);
    ~Display() override = default;

    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;

    bool init() override;
    bool showFramebuffer(const uint8_t* framebuffer, const size_t len) override;
    bool showWindow(const uint8_t col0, const uint8_t col1, const uint8_t page0,
                    const uint8_t page1, const uint8_t* data, const size_t len) override;

   private:
    bool writeCommand(const uint8_t* cmd, const size_t len);
    bool writeData(const uint8_t* data, const size_t len);

   private:
    II2cBus& mI2cBus;
    const uint8_t mI2cAddr;
    bool mReady{false};
};

}  // namespace adapters
