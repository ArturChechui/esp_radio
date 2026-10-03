/**
 * @file RingBuffer.hpp
 * @brief Concrete implementation of a thread-safe circular buffer.
 *
 * This file contains the RingBuffer class, which implements the IRingBuffer interface. It is
 * designed for single-producer, single-consumer scenarios, such as audio streaming pipelines.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "IRingBuffer.hpp"
#include "PsramAllocator.hpp"
#include "Signal.hpp"

namespace common {

namespace {
// Standard cache line size for ESP32 / Xtensa LX architecture
constexpr std::size_t HwCacheLineSize = 32U;
}  // namespace

/**
 * @class RingBuffer
 * @brief Single-producer, single-consumer thread-safe circular buffer implementation.
 *
 * This implementation is designed for the "audio pipeline" use case:
 * - 1 producer task (e.g., HTTP reader)
 * - 1 consumer task (e.g., decoder/player)
 *
 * It provides thread safety via an atomic approach and offers zero-copy spans to allow
 * producers and consumers to access the internal memory directly, avoiding extra temporary buffers.
 */
class RingBuffer : public IRingBuffer {
   public:
    /**
     * @brief Constructs a RingBuffer with a fixed capacity.
     * @param size The total capacity of the buffer in bytes.
     */
    explicit RingBuffer(const size_t size);

    /** @brief Default virtual destructor. */
    ~RingBuffer() override = default;

    /** @name Zero-copy APIs
     * Methods providing direct access to internal memory segments.
     * @{ */

    /**
     * @brief Requests read-only access to internal data segments.
     * @param maxBytes Maximum number of bytes to claim.
     * @return ReadSpans containing pointers to the data. This does NOT advance the read position.
     */
    ReadSpans claimReadSpans(const size_t maxBytes) const override;

    /**
     * @brief Commits a read operation and advances the internal read pointer.
     * @param bytes The number of bytes successfully processed.
     */
    void commitRead(size_t bytes) override;

    /**
     * @brief Requests writable access to internal free space.
     * @param maxBytes Maximum number of bytes to claim.
     * @return WriteSpans containing pointers to writable segments. This does NOT advance the write
     * position.
     */
    WriteSpans claimWriteSpans(const size_t maxBytes) override;

    /**
     * @brief Commits a write operation and advances the internal write pointer.
     * @param bytes The number of bytes successfully written.
     */
    void commitWrite(size_t bytes) override;
    /** @} */

    /** @name Blocking Helpers
     * Synchronization methods used to wait for buffer state changes.
     * @{ */

    /**
     * @brief Blocks until the buffer contains at least 1 byte of data.
     * @param timeoutMs Maximum time to wait.
     * @return true if data is available, false on timeout.
     */
    bool waitForData(const uint32_t timeoutMs) override;

    /**
     * @brief Blocks until the buffer has at least 1 byte of free space.
     * @param timeoutMs Maximum time to wait.
     * @return true if space is available, false on timeout.
     */
    bool waitForSpace(const uint32_t timeoutMs) override;
    /** @} */

    /** @brief Returns the number of bytes currently available for reading.
     * Optimized for PRODUCER to speed up or slow down based on consumer progress.
     */
    size_t available() const override;

    /** @brief Retrieves the current availability and free space metrics atomically. */
    FillLevels getFillLevels() const override;

    /** @brief Returns the total capacity of the buffer. */
    size_t capacity() const override;

    /** @brief Aborts all pending and future operations, unblocking any waiting tasks. */
    void abort() override;

    /** @brief Resets the buffer to an empty state, clearing all data. */
    void reset() override;

   private:
    size_t mCapacity; /**< Total size of the underlying buffer. */
    std::vector<uint8_t, common::PsramAllocator<uint8_t>>
        mBuffer; /**< Internal storage for the circular data in PSRAM. */

    alignas(HwCacheLineSize) std::atomic<size_t> mWritePos; /**< Current write offset in bytes. */
    alignas(HwCacheLineSize) std::atomic<size_t> mReadPos;  /**< Current read offset in bytes. */
    std::atomic<bool> mAborted; /**< Flag indicating if the buffer has been aborted. */

    mutable common::Signal mDataSignal;  /**< Signal triggered when data is written. */
    mutable common::Signal mSpaceSignal; /**< Signal triggered when space is freed. */
};

}  // namespace common
