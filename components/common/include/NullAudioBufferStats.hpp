#pragma once

#include "IAudioBufferStats.hpp"

namespace common {

class NullAudioBufferStats : public IAudioBufferStats {
   public:
    explicit NullAudioBufferStats(uint32_t = 0) noexcept {}
    ~NullAudioBufferStats() override = default;

    inline void setPeriodMs(uint32_t) noexcept override {}
    inline bool shouldLog() noexcept override {
        return false;
    }

    inline void observeRing(size_t, size_t) noexcept override {}
    inline void onFrameDecoded() noexcept override {}
    inline void onDecodeFrameBytesZero() noexcept override {}
    inline void onInvalidFrameInfo() noexcept override {}
    inline void onZeroSampleFrame() noexcept override {}
    inline void onResyncDrop() noexcept override {}
    inline void onI2sWrite(size_t, bool, bool) noexcept override {}
    inline void onHttpRead(int) noexcept override {}

    inline Snapshot snapshotAndReset() noexcept override {
        return Snapshot{};
    }
};

}  // namespace common
