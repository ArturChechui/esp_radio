/**
 * @file IRingBuffer.hpp
 * @brief Interface for a thread-safe circular buffer.
 *
 * This file defines the IRingBuffer interface, which facilitates efficient
 * data transfer between producer and consumer tasks (e.g., Network and Audio).
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace common {

/**
 * @class IRingBuffer
 * @brief Abstract interface for a circular (ring) buffer.
 *
 * This interface provides dual access patterns:
 * 1. Standard Read/Write: Data is copied between user buffers and internal storage.
 * 2. Zero-Copy Spans: Provides direct pointers to internal memory segments,
 * avoiding unnecessary copies.
 */
class IRingBuffer {
   public:
    /** @brief Represents the current fill state of the buffer. */
    struct FillLevels {
        size_t avail; /**< Bytes currently available for reading. */
        size_t space; /**< Free bytes remaining for writing. */
    };

    /** @brief A read-only segment of memory. */
    struct ConstSpan {
        const uint8_t* ptr{nullptr}; /**< Pointer to the data segment. */
        size_t len{0UL};             /**< Length of the segment in bytes. */
    };

    /** @brief A writable segment of memory. */
    struct Span {
        uint8_t* ptr{nullptr}; /**< Pointer to the data segment. */
        size_t len{0UL};       /**< Length of the segment in bytes. */
    };

    /**
     * @brief Result of a read-span claim.
     * Contains up to two segments if the read data wraps around the buffer end.
     */
    struct ReadSpans {
        ConstSpan first{};  /**< First contiguous read segment. */
        ConstSpan second{}; /**< Second segment (non-empty only when wrapped). */

        /** @brief Calculates the total number of bytes across both segments. */
        size_t total() const {
            return (first.len + second.len);
        }
    };

    /**
     * @brief Result of a write-span claim.
     * Contains up to two segments if the available space wraps around the buffer end.
     */
    struct WriteSpans {
        Span first{};  /**< First contiguous writable segment. */
        Span second{}; /**< Second segment (non-empty only when wrapped). */

        /** @brief Calculates the total capacity across both segments. */
        size_t total() const {
            return (first.len + second.len);
        }
    };

    /** @brief Virtual destructor for safe interface cleanup. */
    virtual ~IRingBuffer() = default;

    /** @name Compatibility Copy APIs
     * Standard methods for moving data by copying it into or out of the buffer.
     * @{ */

    /**
     * @brief Copies data into the ring buffer.
     * @param data Pointer to the source data.
     * @param len Number of bytes to write.
     * @param timeoutMs Maximum time to wait for sufficient space.
     * @return The actual number of bytes written.
     */
    virtual size_t write(const uint8_t* data, const size_t len, const uint32_t timeoutMs) = 0;

    /**
     * @brief Copies data out of the ring buffer.
     * @param data Pointer to the destination buffer.
     * @param len Number of bytes to read.
     * @param timeoutMs Maximum time to wait for sufficient data.
     * @return The actual number of bytes read.
     */
    virtual size_t read(uint8_t* data, const size_t len, const uint32_t timeoutMs) = 0;
    /** @} */

    /** @name Zero-Copy APIs
     * Methods to access the internal buffer memory directly to avoid memcpy overhead.
     * @{ */

    /**
     * @brief Requests read-only access to available data.
     * @param maxBytes Maximum number of bytes to claim.
     * @return ReadSpans containing pointers to the data. This does NOT advance the read position.
     */
    virtual ReadSpans claimReadSpans(const size_t maxBytes) const = 0;

    /**
     * @brief Marks a specific number of bytes as processed and advances the read position.
     * @param bytes Number of bytes to commit.
     */
    virtual void commitRead(size_t bytes) = 0;

    /**
     * @brief Requests writable access to free space.
     * @param maxBytes Maximum number of bytes to claim.
     * @return WriteSpans containing pointers to the free space. This does NOT advance the write
     * position.
     */
    virtual WriteSpans claimWriteSpans(const size_t maxBytes) = 0;

    /**
     * @brief Marks a specific number of bytes as written and advances the write position.
     * @param bytes Number of bytes to commit.
     */
    virtual void commitWrite(size_t bytes) = 0;
    /** @} */

    /** @name Blocking Helpers
     * Methods used by producer/consumer loops to wait for state changes.
     * @{ */

    /**
     * @brief Blocks until the buffer contains data or the timeout expires.
     * @param timeoutMs Maximum wait time.
     * @return true if data is available, false on timeout.
     */
    virtual bool waitForData(const uint32_t timeoutMs) = 0;

    /**
     * @brief Blocks until the buffer has free space or the timeout expires.
     * @param timeoutMs Maximum wait time.
     * @return true if space is available, false on timeout.
     */
    virtual bool waitForSpace(const uint32_t timeoutMs) = 0;
    /** @} */

    /**
     * @brief Returns the number of bytes currently available for reading.
     */
    virtual size_t available() const = 0;

    /**
     * @brief Returns the number of bytes currently free for writing.
     */
    virtual size_t space() const = 0;

    /**
     * @brief Retrieves both available data and free space in a single atomic-like call.
     */
    virtual FillLevels getFillLevels() const = 0;

    /**
     * @brief Returns the total capacity of the buffer.
     */
    virtual size_t capacity() const = 0;

    /**
     * @brief Aborts all pending and future operations, unblocking any waiting tasks.
     */
    virtual void abort() = 0;

    /**
     * @brief Resets the buffer to an empty state, clearing all data.
     */
    virtual void reset() = 0;
};

}  // namespace common
