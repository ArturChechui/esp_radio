/**
 * @file IAudioBufferStats.hpp
 * @brief Interface for tracking and reporting audio pipeline statistics.
 *
 * This file defines the Snapshot structure and the IAudioBufferStats interface
 * used to monitor the performance of the audio ring buffer, MP3 decoder,
 * I2S output, and HTTP stream.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace common {

/**
 * @struct Snapshot
 * @brief A container for a point-in-time collection of audio performance metrics.
 *
 * This structure aggregates counters and gauges from different parts of the
 * audio system, including buffer levels, decoder errors, and network throughput.
 */
struct Snapshot {
    // Period info
    uint32_t period_ms{0}; /**< The duration in milliseconds covered by this snapshot. */

    // Ring (player-owned)
    size_t avail_now{0}; /**< Bytes currently available in the ring buffer. */
    size_t min_avail{0}; /**< Minimum bytes available during the period. */
    size_t max_avail{0}; /**< Maximum bytes available during the period. */
    size_t min_space{0}; /**< Minimum free space in the buffer during the period. */

    // Decode (player-owned)
    uint32_t frames{0};             /**< Total number of successfully decoded frames. */
    uint32_t resync_drops{0};       /**< Total bytes or frames dropped due to resynchronization. */
    uint32_t decode_frame0{0};      /**< Count of frames that decoded to zero PCM bytes. */
    uint32_t invalid_frame_info{0}; /**< Count of frames with invalid header information. */
    uint32_t zero_sample_frames{
        0}; /**< Count of frames successfully decoded but containing zero samples. */

    // I2S (player-owned)
    uint32_t i2s_calls{0};                    /**< Total number of I2S write attempts. */
    uint32_t i2s_timeouts{0};                 /**< Total number of I2S hardware timeouts. */
    uint32_t i2s_written_bytes{0};            /**< Total bytes successfully written to I2S. */
    uint32_t i2s_min_written_bytes{0};        /**< Minimum bytes written in a single I2S call. */
    uint32_t i2s_max_written_bytes{0};        /**< Maximum bytes written in a single I2S call. */
    uint32_t i2s_partial_writes{0};           /**< Count of incomplete (partial) I2S writes. */
    uint32_t i2s_max_consecutive_timeouts{0}; /**< Longest sequence of back-to-back I2S timeouts. */

    // HTTP (shared via mux)
    uint32_t http_calls{0};  /**< Total number of HTTP read operations. */
    uint32_t http_zero{0};   /**< Count of HTTP reads that returned zero bytes. */
    uint32_t http_errors{0}; /**< Total number of network/HTTP errors. */
    uint32_t http_bytes{0};  /**< Total raw bytes fetched from the network. */
};

/**
 * @class IAudioBufferStats
 * @brief Abstract interface for a statistics collector.
 *
 * Objects implementing this interface are used to record events and
 * telemetry from both the audio player task and the network task.
 */
class IAudioBufferStats {
   public:
    /** @brief Virtual destructor for safe interface cleanup. */
    virtual ~IAudioBufferStats() = default;

    /**
     * @brief Configures the reporting interval.
     * @param periodMs Interval in milliseconds.
     */
    virtual void setPeriodMs(uint32_t periodMs) = 0;

    /**
     * @brief Checks if the reporting interval has passed.
     * @return true if a snapshot should be taken.
     */
    virtual bool shouldLog() = 0;

    /**
     * @brief Records current ring buffer occupancy.
     * @param avail Bytes currently available.
     * @param space Bytes currently free.
     */
    virtual void observeRing(size_t avail, size_t space) = 0;

    /** @brief Increments successfully decoded frame count. */
    virtual void onFrameDecoded() = 0;

    /** @brief Records a frame that decoded to zero PCM bytes. */
    virtual void onDecodeFrameBytesZero() = 0;

    /** @brief Records an invalid MP3 frame header. */
    virtual void onInvalidFrameInfo() = 0;

    /** @brief Records a frame with zero audio samples. */
    virtual void onZeroSampleFrame() = 0;

    /** @brief Records a resynchronization drop event. */
    virtual void onResyncDrop() = 0;

    /**
     * @brief Records results of an I2S hardware write.
     * @param writtenBytes Bytes successfully sent.
     * @param timeoutOrZero True if the call timed out or sent no data.
     * @param partialWrite True if the write was incomplete.
     */
    virtual void onI2sWrite(size_t writtenBytes, bool timeoutOrZero, bool partialWrite) = 0;

    /**
     * @brief Captures current metrics into a Snapshot and resets interval counters.
     * @return The populated Snapshot object.
     */
    virtual Snapshot snapshotAndReset() = 0;

    /**
     * @brief Records an HTTP read operation result.
     * @param r The number of bytes read or an error code.
     */
    virtual void onHttpRead(int r) = 0;
};

}  // namespace common
