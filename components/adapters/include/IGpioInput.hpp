#pragma once

namespace adapters {
class IGpioInput {
   public:
    virtual ~IGpioInput() = default;

    virtual bool init() = 0;
    virtual void deinit() = 0;
    virtual int getLevel(const uint32_t gpioNum) = 0;
};
}  // namespace adapters
