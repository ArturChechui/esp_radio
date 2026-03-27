#pragma once

#include <cstdint>

namespace services {
constexpr uint16_t NightLux = 5U;

class IInputService {
   public:
    virtual ~IInputService() = default;

    virtual bool init() = 0;
    virtual void deinit() = 0;
    virtual void setMode(const bool night) = 0;
};

}  // namespace services
