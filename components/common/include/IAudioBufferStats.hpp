#pragma once

#include <cstddef>
#include <cstdint>

namespace common {
struct Snapshot {
    // Period info
    uint32_t period_ms{0};

    // Ring (player-owned)
    size_t avail_now{0};
    size_t min_avail{0};
    size_t max_avail{0};
    size_t min_space{0};

    // Decode (player-owned)
    uint32_t frames{0};
    uint32_t resync_drops{0};
    uint32_t decode_frame0{0};
    uint32_t invalid_frame_info{0};
    uint32_t zero_sample_frames{0};

    // I2S (player-owned)
    uint32_t i2s_calls{0};
    uint32_t i2s_timeouts{0};
    uint32_t i2s_written_bytes{0};
    uint32_t i2s_min_written_bytes{0};
    uint32_t i2s_max_written_bytes{0};
    uint32_t i2s_partial_writes{0};
    uint32_t i2s_max_consecutive_timeouts{0};

    // HTTP (shared via mux)
    uint32_t http_calls{0};
    uint32_t http_zero{0};
    uint32_t http_errors{0};
    uint32_t http_bytes{0};
};

class IAudioBufferStats {
   public:
    virtual ~IAudioBufferStats() = default;

    virtual void setPeriodMs(uint32_t periodMs) = 0;
    virtual bool shouldLog() = 0;
    virtual void observeRing(size_t avail, size_t space) = 0;
    virtual void onFrameDecoded() = 0;
    virtual void onDecodeFrameBytesZero() = 0;
    virtual void onInvalidFrameInfo() = 0;
    virtual void onZeroSampleFrame() = 0;
    virtual void onResyncDrop() = 0;
    virtual void onI2sWrite(size_t writtenBytes, bool timeoutOrZero, bool partialWrite) = 0;

    virtual Snapshot snapshotAndReset() = 0;
    virtual void onHttpRead(int r) = 0;
};

}  // namespace common
