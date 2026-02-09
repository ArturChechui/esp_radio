#pragma once

#include <cstdint>

namespace common {
class IRingBuffer {
   public:
    struct FillLevels {
        size_t avail;
        size_t space;
    };

    struct ConstSpan {
        const uint8_t* ptr{nullptr};
        size_t len{0UL};
    };
    struct Span {
        uint8_t* ptr{nullptr};
        size_t len{0UL};
    };

    struct ReadSpans {
        ConstSpan first{};
        ConstSpan second{};  // non-empty only when wrapped
        size_t total() const {
            return (first.len + second.len);
        }
    };

    struct WriteSpans {
        Span first{};
        Span second{};  // non-empty only when wrapped
        size_t total() const {
            return (first.len + second.len);
        }
    };

    virtual ~IRingBuffer() = default;

    // Compatibility copy APIs
    virtual size_t write(const uint8_t* data, const size_t len, const uint32_t timeoutMs) = 0;
    virtual size_t read(uint8_t* data, const size_t len, const uint32_t timeoutMs) = 0;

    // Zero-copy APIs
    virtual ReadSpans claimReadSpans(const size_t maxBytes) const = 0;  // does NOT advance read pos
    virtual void commitRead(size_t bytes) = 0;                          // advances read pos

    virtual WriteSpans claimWriteSpans(const size_t maxBytes) = 0;  // does NOT advance write pos
    virtual void commitWrite(size_t bytes) = 0;                     // advances write pos

    // Blocking helpers (used by producer/consumer loops)
    virtual bool waitForData(const uint32_t timeoutMs) = 0;
    virtual bool waitForSpace(const uint32_t timeoutMs) = 0;

    // available data for reading
    virtual size_t available() const = 0;

    // free space for writing
    virtual size_t space() const = 0;

    virtual FillLevels getFillLevels() const = 0;

    virtual size_t capacity() const = 0;

    virtual void abort() = 0;
    virtual void reset() = 0;
};

}  // namespace common
