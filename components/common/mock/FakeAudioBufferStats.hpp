#pragma once

#include <cstddef>
#include <cstdint>

#include "IAudioBufferStats.hpp"

namespace common {

class FakeAudioBufferStats : public IAudioBufferStats {
   public:
    explicit FakeAudioBufferStats(uint32_t periodMs) {
        (void)periodMs;
    }
    ~FakeAudioBufferStats() override = default;

    void setPeriodMs(uint32_t periodMs) override {
        (void)periodMs;
    }
    bool shouldLog() override {
        return false;
    }
    void observeRing(size_t avail, size_t space) override {
        (void)avail;
        (void)space;
    }
    void onFrameDecoded() override {}
    void onDecodeFrameBytesZero() override {}
    void onInvalidFrameInfo() override {}
    void onZeroSampleFrame() override {}
    void onResyncDrop() override {}
    void onI2sWrite(size_t writtenBytes, bool timeoutOrZero, bool partialWrite) override {
        (void)writtenBytes;
        (void)timeoutOrZero;
        (void)partialWrite;
    }
    Snapshot snapshotAndReset() override {
        return {};
    }
    void onHttpRead(int r) override {
        (void)r;
    }
};

}  // namespace common
