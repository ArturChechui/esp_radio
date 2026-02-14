#pragma once

#include <cstddef>
#include <cstdint>

namespace adapters {

class IDisplay {
   public:
    virtual ~IDisplay() = default;

    virtual bool init() = 0;

    // Full framebuffer write (SSD1306 page-major layout: pages first, then columns).
    virtual bool showFramebuffer(const uint8_t* data, const size_t len) = 0;

    // Window write using SSD1306 column/page addressing.
    // Data layout: for p in [page0..page1], for x in [col0..col1], push byte.
    virtual bool showWindow(const uint8_t col0, const uint8_t col1, const uint8_t page0,
                            const uint8_t page1, const uint8_t* data, const size_t len) = 0;
};

}  // namespace adapters
