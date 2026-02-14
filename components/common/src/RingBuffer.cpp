#include "RingBuffer.hpp"

#include <esp_log.h>

#include <algorithm>
#include <cstring>

#include "Helper.hpp"
#include "LockGuard.hpp"

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
      mMutex(),
      mDataSignal(),
      mSpaceSignal() {
    if (!mMutex.isValid() || !mDataSignal.isValid() || !mSpaceSignal.isValid()) {
        ESP_LOGE(Tag, "Failed to create semaphores");
    }
}

size_t RingBuffer::write(const uint8_t* data, const size_t len, const uint32_t timeoutMs) {
    if (!data || len == 0UL) {
        return 0UL;
    }

    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout = toTicks(timeoutMs);

    size_t writtenTotal = 0UL;
    while (writtenTotal < len) {
        const size_t bytesToWrite = (len - writtenTotal);
        const WriteSpans spans = claimWriteSpans(bytesToWrite);
        const size_t totalSpace = spans.total();

        // No space available, wait
        if (totalSpace == 0UL) {
            TickType_t remaining = portMAX_DELAY;
            if (timeout != portMAX_DELAY) {
                const TickType_t now = xTaskGetTickCount();
                const TickType_t elapsed = (now - start);
                if (elapsed >= timeout) {
                    break;
                }
                remaining = (timeout - elapsed);
            }

            // TODO: replace with normal impl, calculate in ms and then provide in ms
            if (mSpaceSignal.wait(pdTICKS_TO_MS(remaining)) == false) {
                break;
            }
            continue;
        }

        size_t written = 0UL;
        if (0UL < spans.first.len) {
            std::memcpy(spans.first.ptr, (data + writtenTotal), spans.first.len);
            written += spans.first.len;
        }
        if (0UL < spans.second.len) {
            std::memcpy(spans.second.ptr, (data + writtenTotal + written), spans.second.len);
            written += spans.second.len;
        }

        commitWrite(written);
        writtenTotal += written;
    }

    return writtenTotal;
}

size_t RingBuffer::read(uint8_t* data, const size_t len, const uint32_t timeoutMs) {
    if (!data || len == 0UL) {
        return 0UL;
    }

    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout = toTicks(timeoutMs);

    while (true) {
        const ReadSpans spans = claimReadSpans(len);
        const size_t totalAvailable = spans.total();

        // No data available, wait
        if (totalAvailable == 0UL) {
            TickType_t remaining = portMAX_DELAY;
            if (timeout != portMAX_DELAY) {
                const TickType_t now = xTaskGetTickCount();
                const TickType_t elapsed = (now - start);
                if (elapsed >= timeout) {
                    return 0UL;
                }
                remaining = (timeout - elapsed);
            }

            if (mDataSignal.wait(pdTICKS_TO_MS(remaining)) == false) {
                return 0UL;
            }
            continue;
        }

        size_t read = 0UL;
        if (0UL < spans.first.len) {
            std::memcpy(data, spans.first.ptr, spans.first.len);
            read += spans.first.len;
        }
        if (0UL < spans.second.len) {
            std::memcpy((data + read), spans.second.ptr, spans.second.len);
            read += spans.second.len;
        }
        commitRead(read);

        return read;
    }
}

RingBuffer::ReadSpans RingBuffer::claimReadSpans(const size_t maxBytes) const {
    ReadSpans out{};

    if (maxBytes == 0UL) {
        return out;
    }

    LockGuard lock(mMutex);
    if (mAborted) {
        return out;
    }

    const size_t avail = availableUnlocked();
    const size_t bytesToRead = std::min(avail, maxBytes);
    if (bytesToRead == 0UL) {
        return out;
    }

    if (mWritePos >= mReadPos) {
        // Single contiguous region [read..write)
        out.first.ptr = (mBuffer.data() + mReadPos);
        out.first.len = bytesToRead;
    } else {
        // Wrapped Case: [read..end) + [0..write)
        const size_t firstLen = std::min(bytesToRead, (mCapacity - mReadPos));
        out.first.ptr = (mBuffer.data() + mReadPos);
        out.first.len = firstLen;

        const size_t remaining = (bytesToRead - firstLen);
        if (remaining > 0UL) {
            out.second.ptr = mBuffer.data();
            out.second.len = std::min(remaining, mWritePos);
        }
    }

    return out;
}

void RingBuffer::commitRead(size_t bytes) {
    if (bytes == 0UL) {
        return;
    }

    bool wasFull = false;

    {
        LockGuard lock(mMutex);
        if (mAborted) {
            return;
        }

        // If space was 0, producer may be blocked waiting for space.
        wasFull = (spaceUnlocked() == 0UL);

        const size_t avail = availableUnlocked();
        if (bytes > avail) {
            bytes = avail;  // clamp defensive
        }

        advanceReadUnlocked(bytes);
    }

    if (wasFull) {
        mSpaceSignal.signal();
    }
}

RingBuffer::WriteSpans RingBuffer::claimWriteSpans(const size_t maxBytes) {
    WriteSpans out{};

    if (maxBytes == 0UL) {
        return out;
    }

    LockGuard lock(mMutex);
    if (mAborted) {
        return out;
    }

    const size_t bytesToWrite = std::min(spaceUnlocked(), maxBytes);
    if (bytesToWrite == 0UL) {
        return out;
    }

    const bool contiguousCase = ((mWritePos < mReadPos) || (mReadPos == 0UL));
    if (contiguousCase) {
        // Single contiguous free region: [write..read-Guard) or [write..end-Guard) if read==0
        const size_t maxFirst = (mReadPos > mWritePos) ? (mReadPos - mWritePos - Guard)
                                                       : (mCapacity - mWritePos - Guard);
        out.first.ptr = (mBuffer.data() + mWritePos);
        out.first.len = std::min(bytesToWrite, maxFirst);
    } else {
        // Wrapped free region: [write..end) + [0..read-Guard)
        const size_t maxFirst = (mCapacity - mWritePos);
        const size_t firstLen = std::min(bytesToWrite, maxFirst);
        out.first.ptr = (mBuffer.data() + mWritePos);
        out.first.len = firstLen;

        const size_t remaining = (bytesToWrite - firstLen);
        if (remaining > 0UL) {
            out.second.ptr = mBuffer.data();
            out.second.len = std::min(remaining, (mReadPos - Guard));
        }
    }

    return out;
}

void RingBuffer::commitWrite(size_t bytes) {
    if (bytes == 0UL) {
        return;
    }

    bool wasEmpty = false;

    {
        LockGuard lock(mMutex);
        if (mAborted) {
            return;
        }

        wasEmpty = (availableUnlocked() == 0UL);

        const size_t freeTotal = spaceUnlocked();
        if (bytes > freeTotal) {
            bytes = freeTotal;  // clamp defensive
        }

        advanceWriteUnlocked(bytes);
    }

    if (wasEmpty) {
        mDataSignal.signal();
    }
}

bool RingBuffer::waitForData(const uint32_t timeoutMs) {
    {
        LockGuard lock(mMutex);
        if (mAborted) {
            return false;
        }
        if (availableUnlocked() > 0UL) {
            return true;
        }
    }

    return mDataSignal.wait(timeoutMs);
}

bool RingBuffer::waitForSpace(const uint32_t timeoutMs) {
    {
        LockGuard lock(mMutex);
        if (mAborted) {
            return false;
        }
        if (spaceUnlocked() > 0UL) {
            return true;
        }
    }

    return mSpaceSignal.wait(timeoutMs);
}

size_t RingBuffer::available() const {
    LockGuard lock(mMutex);

    return availableUnlocked();
}

size_t RingBuffer::space() const {
    LockGuard lock(mMutex);

    return spaceUnlocked();
}

IRingBuffer::FillLevels RingBuffer::getFillLevels() const {
    LockGuard lock(mMutex);

    return {availableUnlocked(), spaceUnlocked()};
}

size_t RingBuffer::capacity() const {
    return mCapacity;
}

void RingBuffer::abort() {
    ESP_LOGW(Tag, "abort()");

    {
        LockGuard lock(mMutex);
        mAborted = true;
    }

    // Unblock any pending waits
    mDataSignal.signal();
    mSpaceSignal.signal();
}

void RingBuffer::reset() {
    ESP_LOGI(Tag, "reset()");

    {
        LockGuard lock(mMutex);
        mReadPos = 0UL;
        mWritePos = 0UL;
        mAborted = false;
    }

    // Unblock any pending waits
    mDataSignal.signal();
    mSpaceSignal.signal();
}

size_t RingBuffer::availableUnlocked() const {
    if (mWritePos >= mReadPos) {
        return mWritePos - mReadPos;
    }

    return (mCapacity - mReadPos) + mWritePos;
}

size_t RingBuffer::spaceUnlocked() const {
    return ((mCapacity - availableUnlocked()) - Guard);
}

void RingBuffer::advanceWriteUnlocked(const size_t bytes) {
    mWritePos += bytes;

    // if write pos reaches capacity, wrap around
    if (mWritePos >= mCapacity) {
        mWritePos %= mCapacity;
    }
}

void RingBuffer::advanceReadUnlocked(const size_t bytes) {
    mReadPos += bytes;

    // if read pos reaches capacity, wrap around
    if (mReadPos >= mCapacity) {
        mReadPos %= mCapacity;
    }
}

}  // namespace common
