#pragma once

#include <cstddef>
#include <cstdint>

namespace adapters {
class IDisplay {
   public:
    virtual ~IDisplay() = default;

    virtual bool init() = 0;
    virtual bool showFramebuffer(const uint8_t* framebuffer, const size_t& len) = 0;
};

}  // namespace adapters
