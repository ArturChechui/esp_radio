#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>  // portMUX_TYPE, portENTER_CRITICAL
#include <freertos/task.h>

#include <cstddef>
#include <cstdint>

#include "IAudioBufferStats.hpp"

namespace common {

/**
 * AudioBufferStats
 *
 * Threading model:
 * - HttpTask calls onHttpRead().
 * - PlayerTask calls observeRing(), onFrameDecoded(), onDecodeFrameBytesZero(), onResyncDrop(),
 *   onI2sWrite(), shouldLog(), and snapshotAndReset().
 *
 * Safety:
 * - Player-side stats are only touched by PlayerTask => no lock needed.
 * - HTTP stats are touched by HttpTask and read/reset by PlayerTask => protected by a tiny mux.
 *
 * Overhead:
 * - Fast inline counters.
 * - Critical section only around 4 integers (HTTP stats), not per-sample work.
 */
class AudioBufferStats : public IAudioBufferStats {
   public:
    explicit AudioBufferStats(uint32_t periodMs)
        : mPeriodTicks(pdMS_TO_TICKS(periodMs)),
          mPeriodMs(periodMs),
          mLastLogTick(xTaskGetTickCount()) {
        resetPlayerInterval();
        resetHttpInterval();
    }

    ~AudioBufferStats() override = default;

    void setPeriodMs(uint32_t periodMs) override {
        mPeriodTicks = pdMS_TO_TICKS(periodMs);
        mPeriodMs = periodMs;
        mLastLogTick = xTaskGetTickCount();
    }

    // -------- PlayerTask API --------

    // Call frequently in PlayerTask; returns true once per period.
    bool shouldLog() override {
        const TickType_t now = xTaskGetTickCount();
        if ((now - mLastLogTick) >= mPeriodTicks) {
            mLastLogTick = now;
            return true;
        }
        return false;
    }

    void observeRing(size_t avail, size_t space) override {
        mAvailNow = avail;
        if (avail < mMinAvail)
            mMinAvail = avail;
        if (avail > mMaxAvail)
            mMaxAvail = avail;
        if (space < mMinSpace)
            mMinSpace = space;
    }

    void onFrameDecoded() override {
        ++mFrames;
    }

    void onDecodeFrameBytesZero() override {
        ++mDecodeFrame0;
    }

    void onInvalidFrameInfo() override {
        ++mInvalidFrameInfo;
    }
    void onZeroSampleFrame() override {
        ++mZeroSampleFrames;
    }

    void onResyncDrop() override {
        ++mResyncDrops;
    }

    // timeoutOrZero=true when i2s write returned 0 OR your bus reports timeout
    void onI2sWrite(size_t writtenBytes, bool timeoutOrZero, bool partialWrite) override {
        ++mI2sCalls;
        mI2sWrittenBytes += static_cast<uint32_t>(writtenBytes);

        // min is valid only when it is not timeout or zero
        if (writtenBytes < mMinI2sWrittenBytes && !timeoutOrZero) {
            mMinI2sWrittenBytes = writtenBytes;
        }
        if (writtenBytes > mMaxI2sWrittenBytes) {
            mMaxI2sWrittenBytes = writtenBytes;
        }

        if (timeoutOrZero) {
            ++mI2sTimeouts;
            ++mConsecutiveI2sTimeouts;
            if (mConsecutiveI2sTimeouts > mI2sMaxConsecutiveTimeouts) {
                mI2sMaxConsecutiveTimeouts = mConsecutiveI2sTimeouts;
            }
        } else {
            mConsecutiveI2sTimeouts = 0;
        }

        if (partialWrite) {
            ++mI2sPartialWrites;
        }
    }

    // Snapshot all counters and reset interval counters.
    // Call this ONLY from PlayerTask, typically right before logging.
    Snapshot snapshotAndReset() override {
        Snapshot s{};
        s.period_ms = mPeriodMs;

        // Player-owned fields
        s.avail_now = mAvailNow;
        s.min_avail = (mMinAvail == kSizeMax) ? 0 : mMinAvail;
        s.max_avail = mMaxAvail;
        s.min_space = (mMinSpace == kSizeMax) ? 0 : mMinSpace;

        s.frames = mFrames;
        s.resync_drops = mResyncDrops;
        s.decode_frame0 = mDecodeFrame0;
        s.invalid_frame_info = mInvalidFrameInfo;
        s.zero_sample_frames = mZeroSampleFrames;

        s.i2s_calls = mI2sCalls;
        s.i2s_timeouts = mI2sTimeouts;
        s.i2s_written_bytes = mI2sWrittenBytes;
        s.i2s_min_written_bytes = (mMinI2sWrittenBytes == kSizeMax) ? 0 : mMinI2sWrittenBytes;
        s.i2s_max_written_bytes = mMaxI2sWrittenBytes;

        s.i2s_partial_writes = mI2sPartialWrites;
        s.i2s_max_consecutive_timeouts = mI2sMaxConsecutiveTimeouts;

        // Shared HTTP fields (copy+reset under tiny critical section)
        portENTER_CRITICAL(&mHttpMux);
        s.http_calls = mHttpCalls;
        s.http_zero = mHttpZero;
        s.http_errors = mHttpErrors;
        s.http_bytes = mHttpBytes;

        resetHttpIntervalNoLock();
        portEXIT_CRITICAL(&mHttpMux);

        resetPlayerInterval();
        return s;
    }

    // -------- HttpTask API --------

    // Call from HttpTask after each readStream().
    // r > 0: bytes
    // r == 0: no data/timeout (depends on your HttpClient behavior)
    // r < 0: error
    void onHttpRead(int r) override {
        portENTER_CRITICAL(&mHttpMux);
        ++mHttpCalls;
        if (r > 0) {
            mHttpBytes += static_cast<uint32_t>(r);
        } else if (r == 0) {
            ++mHttpZero;
        } else {
            ++mHttpErrors;
        }
        portEXIT_CRITICAL(&mHttpMux);
    }

   private:
    static constexpr size_t kSizeMax = static_cast<size_t>(-1);

    void resetPlayerInterval() {
        mAvailNow = 0;
        mMinAvail = kSizeMax;
        mMaxAvail = 0;
        mMinSpace = kSizeMax;

        mFrames = 0;
        mResyncDrops = 0;
        mDecodeFrame0 = 0;
        mZeroSampleFrames = 0;
        mInvalidFrameInfo = 0;

        mI2sCalls = 0;
        mI2sTimeouts = 0;
        mI2sWrittenBytes = 0;
        mMinI2sWrittenBytes = kSizeMax;
        mMaxI2sWrittenBytes = 0;
        mI2sPartialWrites = 0;
        mConsecutiveI2sTimeouts = 0;
        mI2sMaxConsecutiveTimeouts = 0;
    }

    void resetHttpInterval() {
        portENTER_CRITICAL(&mHttpMux);
        resetHttpIntervalNoLock();
        portEXIT_CRITICAL(&mHttpMux);
    }

    void resetHttpIntervalNoLock() {
        mHttpCalls = 0;
        mHttpZero = 0;
        mHttpErrors = 0;
        mHttpBytes = 0;
    }

   private:
    // Period / timing (player-owned)
    TickType_t mPeriodTicks;
    uint32_t mPeriodMs;
    TickType_t mLastLogTick;

    // Player-owned stats
    size_t mAvailNow{0};
    size_t mMinAvail{kSizeMax};
    size_t mMaxAvail{0};
    size_t mMinSpace{kSizeMax};

    uint32_t mFrames{0};
    uint32_t mResyncDrops{0};
    uint32_t mDecodeFrame0{0};
    uint32_t mInvalidFrameInfo{0};
    uint32_t mZeroSampleFrames{0};

    uint32_t mI2sCalls{0};
    uint32_t mI2sTimeouts{0};
    uint32_t mI2sWrittenBytes{0};
    uint32_t mMinI2sWrittenBytes{kSizeMax};
    uint32_t mMaxI2sWrittenBytes{0};
    uint32_t mI2sPartialWrites{0};
    uint32_t mConsecutiveI2sTimeouts{0};
    uint32_t mI2sMaxConsecutiveTimeouts{0};

    // HTTP shared stats
    portMUX_TYPE mHttpMux = portMUX_INITIALIZER_UNLOCKED;
    uint32_t mHttpCalls{0};
    uint32_t mHttpZero{0};
    uint32_t mHttpErrors{0};
    uint32_t mHttpBytes{0};
};

}  // namespace common
