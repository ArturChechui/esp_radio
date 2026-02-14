#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "IRingBuffer.hpp"
#include "Mutex.hpp"
#include "Signal.hpp"

namespace common {
/**
 * RingBuffer optimized for the "audio pipeline" case:
 * - 1 producer task (HTTP reader)
 * - 1 consumer task (decoder/player)
 *
 * Thread-safe via internal mutex, but provides *zero-copy spans* so producer/consumer
 * can avoid extra temporary buffers and expensive memmove/insert loops.
 *
 * Important rules for spans:
 * - claimReadSpans()/claimWriteSpans() return pointers into internal storage.
 * - Use the spans immediately; do not keep them across calls that may advance positions.
 * - After commitRead()/commitWrite(), previously returned spans must be considered invalid.
 */
class RingBuffer : public IRingBuffer {
   public:
    explicit RingBuffer(const size_t size);
    ~RingBuffer() override = default;

    // Compatibility copy APIs
    size_t write(const uint8_t* data, const size_t len, const uint32_t timeoutMs) override;
    size_t read(uint8_t* data, const size_t len, const uint32_t timeoutMs) override;

    // Zero-copy APIs
    ReadSpans claimReadSpans(const size_t maxBytes) const override;  // does NOT advance read pos
    void commitRead(size_t bytes) override;                          // advances read pos

    WriteSpans claimWriteSpans(const size_t maxBytes) override;  // does NOT advance write pos
    void commitWrite(size_t bytes) override;                     // advances write pos

    // Blocking helpers (used by producer/consumer loops)
    bool waitForData(const uint32_t timeoutMs) override;
    bool waitForSpace(const uint32_t timeoutMs) override;

    // available data for reading
    size_t available() const override;

    // free space for writing
    size_t space() const override;

    FillLevels getFillLevels() const override;

    size_t capacity() const override;

    void abort() override;
    void reset() override;

   private:
    size_t availableUnlocked() const;
    size_t spaceUnlocked() const;
    void advanceReadUnlocked(const size_t bytes);
    void advanceWriteUnlocked(const size_t bytes);

    size_t mCapacity;
    std::vector<uint8_t> mBuffer;

    size_t mWritePos;
    size_t mReadPos;
    bool mAborted;

    mutable common::Mutex mMutex;
    mutable common::Signal mDataSignal;
    mutable common::Signal mSpaceSignal;
};

}  // namespace common
