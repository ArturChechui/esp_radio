#include "RingBuffer.hpp"

#include <esp_log.h>

#include <algorithm>
#include <cstring>

namespace {
constexpr const char* Tag = "RingBuffer";
constexpr size_t Guard = 1UL;  // to distinguish full vs empty
}  // namespace

namespace common {
RingBuffer::RingBuffer(const size_t size)
    : mCapacity(std::max<size_t>(size, 2U)),
      mBuffer(mCapacity),
      mWritePos(0U),
      mReadPos(0U),
      mAborted(false),
      mDataSignal(),
      mSpaceSignal() {
    if (!mDataSignal.isValid() || !mSpaceSignal.isValid()) {
        ESP_LOGE(Tag, "Failed to create semaphores");
    }
}

RingBuffer::ReadSpans RingBuffer::claimReadSpans(const size_t maxBytes) const {
    ReadSpans out{};

    if (maxBytes == 0UL || mAborted.load(std::memory_order_acquire)) {
        return out;
    }

    const size_t w = mWritePos.load(std::memory_order_acquire);
    const size_t r = mReadPos.load(std::memory_order_relaxed);
    const size_t avail = (w >= r) ? (w - r) : ((mCapacity - r) + w);

    const size_t bytesToRead = std::min(avail, maxBytes);
    if (bytesToRead == 0UL) {
        return out;
    }

    if (w >= r) {
        // Single contiguous region [read..write)
        out.first.ptr = (mBuffer.data() + r);
        out.first.len = bytesToRead;
    } else {
        // Wrapped Case: [read..end) + [0..write)
        const size_t firstLen = std::min(bytesToRead, (mCapacity - r));
        out.first.ptr = (mBuffer.data() + r);
        out.first.len = firstLen;

        const size_t remaining = (bytesToRead - firstLen);
        if (remaining > 0UL) {
            out.second.ptr = mBuffer.data();
            out.second.len = std::min(remaining, w);
        }
    }

    return out;
}

void RingBuffer::commitRead(size_t bytes) {
    if (bytes == 0UL || mAborted.load(std::memory_order_acquire)) {
        return;
    }

    const size_t w = mWritePos.load(std::memory_order_acquire);
    const size_t r = mReadPos.load(std::memory_order_relaxed);
    const size_t avail = (w >= r) ? (w - r) : ((mCapacity - r) + w);

    if (bytes > avail) {
        bytes = avail;
    }

    size_t newReadPos = (r + bytes);
    if (newReadPos >= mCapacity) {
        newReadPos %= mCapacity;
    }

    mReadPos.store(newReadPos, std::memory_order_release);

    const size_t previousSpace = (mCapacity - avail) - Guard;
    if (previousSpace == 0UL) {
        mSpaceSignal.signal();
    }
}

RingBuffer::WriteSpans RingBuffer::claimWriteSpans(const size_t maxBytes) {
    WriteSpans out{};

    if (maxBytes == 0UL || mAborted.load(std::memory_order_acquire)) {
        return out;
    }

    const size_t w = mWritePos.load(std::memory_order_relaxed);
    const size_t r = mReadPos.load(std::memory_order_acquire);
    const size_t avail = (w >= r) ? (w - r) : ((mCapacity - r) + w);
    const size_t space = ((mCapacity - avail) - Guard);

    const size_t bytesToWrite = std::min(space, maxBytes);
    if (bytesToWrite == 0UL) {
        return out;
    }

    const bool contiguousCase = ((w < r) || (r == 0UL));
    if (contiguousCase) {
        // Single contiguous free region: [write..read-Guard) or [write..end-Guard) if read==0
        const size_t maxFirst = (r > w) ? (r - w - Guard) : (mCapacity - w - Guard);
        out.first.ptr = (mBuffer.data() + w);
        out.first.len = std::min(bytesToWrite, maxFirst);
    } else {
        // Wrapped free region: [write..end) + [0..read-Guard)
        const size_t maxFirst = (mCapacity - w);
        const size_t firstLen = std::min(bytesToWrite, maxFirst);
        out.first.ptr = (mBuffer.data() + w);
        out.first.len = firstLen;

        const size_t remaining = (bytesToWrite - firstLen);
        if (remaining > 0UL) {
            out.second.ptr = mBuffer.data();
            out.second.len = std::min(remaining, (r - Guard));
        }
    }

    return out;
}

void RingBuffer::commitWrite(size_t bytes) {
    if (bytes == 0UL || mAborted.load(std::memory_order_acquire)) {
        return;
    }

    const size_t w = mWritePos.load(std::memory_order_relaxed);
    const size_t r = mReadPos.load(std::memory_order_acquire);
    const size_t avail = (w >= r) ? (w - r) : ((mCapacity - r) + w);
    const size_t space = ((mCapacity - avail) - Guard);

    if (bytes > space) {
        bytes = space;  // clamp defensive
    }

    size_t newWritePos = (w + bytes);
    if (newWritePos >= mCapacity) {
        newWritePos %= mCapacity;
    }

    mWritePos.store(newWritePos, std::memory_order_release);

    if (avail == 0UL) {
        mDataSignal.signal();
    }
}

bool RingBuffer::waitForData(const uint32_t timeoutMs) {
    if (mAborted.load(std::memory_order_acquire)) {
        return false;
    }

    const size_t w = mWritePos.load(std::memory_order_acquire);
    const size_t r = mReadPos.load(std::memory_order_relaxed);
    const size_t avail = (w >= r) ? (w - r) : ((mCapacity - r) + w);

    if (avail > 0UL) {
        return true;
    }

    return mDataSignal.wait(timeoutMs);
}

bool RingBuffer::waitForSpace(const uint32_t timeoutMs) {
    if (mAborted.load(std::memory_order_acquire)) {
        return false;
    }

    const size_t w = mWritePos.load(std::memory_order_relaxed);
    const size_t r = mReadPos.load(std::memory_order_acquire);
    const size_t avail = (w >= r) ? (w - r) : ((mCapacity - r) + w);
    const size_t space = ((mCapacity - avail) - Guard);

    if (space > 0UL) {
        return true;
    }

    return mSpaceSignal.wait(timeoutMs);
}

size_t RingBuffer::available() const {
    const size_t w = mWritePos.load(std::memory_order_relaxed);
    const size_t r = mReadPos.load(std::memory_order_acquire);
    const size_t avail = (w >= r) ? (w - r) : ((mCapacity - r) + w);

    return avail;
}

IRingBuffer::FillLevels RingBuffer::getFillLevels() const {
    const size_t w = mWritePos.load(std::memory_order_acquire);
    const size_t r = mReadPos.load(std::memory_order_acquire);
    const size_t avail = (w >= r) ? (w - r) : ((mCapacity - r) + w);
    const size_t space = ((mCapacity - avail) - Guard);

    return {avail, space};
}

size_t RingBuffer::capacity() const {
    return mCapacity;
}

void RingBuffer::abort() {
    ESP_LOGW(Tag, "abort()");

    mAborted.store(true, std::memory_order_release);

    // Unblock any pending waits
    mDataSignal.signal();
    mSpaceSignal.signal();
}

void RingBuffer::reset() {
    ESP_LOGI(Tag, "reset()");

    mReadPos.store(0UL, std::memory_order_release);
    mWritePos.store(0UL, std::memory_order_release);
    mAborted.store(false, std::memory_order_release);

    // Unblock any pending waits
    mDataSignal.signal();
    mSpaceSignal.signal();
}

}  // namespace common
