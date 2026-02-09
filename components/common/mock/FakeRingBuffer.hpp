// FakeRingBuffer.hpp (for unit tests)

#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "IRingBuffer.hpp"  // your interface

namespace common {

class FakeRingBuffer final : public IRingBuffer {
   public:
    explicit FakeRingBuffer(size_t capacityBytes)
        : mCapacity(std::max<size_t>(capacityBytes, 2U)),
          mBuf(mCapacity),
          mWrite(0),
          mRead(0),
          mAborted(false) {}

    ~FakeRingBuffer() override = default;

    size_t write(const uint8_t* data, const size_t len, const uint32_t timeoutMs) override {
        return 0;
    }
    size_t read(uint8_t* data, const size_t len, const uint32_t timeoutMs) override {
        return 0;
    }

    // --- lifecycle ---
    void reset() override {
        mWrite = 0;
        mRead = 0;
        mAborted = false;
    }

    void abort() override {
        mAborted = true;
    }

    // --- state ---
    size_t capacity() const override {
        return mCapacity;
    }

    size_t available() const override {
        return availableUnlocked();
    }

    size_t space() const override {
        return spaceUnlocked();
    }

    FillLevels getFillLevels() const override {
        return {availableUnlocked(), spaceUnlocked()};
    }

    bool waitForData(uint32_t /*timeoutMs*/) override {
        return (!mAborted) && (availableUnlocked() > 0);
    }

    bool waitForSpace(uint32_t /*timeoutMs*/) override {
        return (!mAborted) && (spaceUnlocked() > 0);
    }

    ReadSpans claimReadSpans(size_t maxBytes) const override {
        ReadSpans out{};
        if (mAborted || maxBytes == 0)
            return out;

        const size_t avail = availableUnlocked();
        const size_t bytesToRead = std::min(avail, maxBytes);
        if (bytesToRead == 0)
            return out;

        if (mWrite >= mRead) {
            // contiguous [read .. write)
            out.first.ptr = mBuf.data() + mRead;
            out.first.len = bytesToRead;
        } else {
            // wrapped: [read .. end) + [0 .. write)
            const size_t firstLen = std::min(bytesToRead, mCapacity - mRead);
            out.first.ptr = mBuf.data() + mRead;
            out.first.len = firstLen;

            const size_t remain = bytesToRead - firstLen;
            if (remain > 0) {
                out.second.ptr = mBuf.data();
                out.second.len = std::min(remain, mWrite);
            }
        }
        return out;
    }

    void commitRead(size_t bytes) override {
        if (mAborted || bytes == 0)
            return;

        const size_t avail = availableUnlocked();
        bytes = std::min(bytes, avail);

        mRead += bytes;
        if (mRead >= mCapacity)
            mRead %= mCapacity;
    }

    WriteSpans claimWriteSpans(size_t maxBytes) override {
        WriteSpans out{};
        if (mAborted || maxBytes == 0)
            return out;

        const size_t freeTotal = spaceUnlocked();
        const size_t bytesToWrite = std::min(freeTotal, maxBytes);
        if (bytesToWrite == 0)
            return out;

        const bool contiguousCase = (mWrite < mRead) || (mRead == 0);
        if (contiguousCase) {
            // [write .. read-Guard)  or [write .. end-Guard) if read==0
            const size_t maxFirst =
                (mRead > mWrite) ? (mRead - mWrite - Guard) : (mCapacity - mWrite - Guard);

            out.first.ptr = mBuf.data() + mWrite;
            out.first.len = std::min(bytesToWrite, maxFirst);
        } else {
            // wrapped free: [write .. end) + [0 .. read-Guard)
            const size_t maxFirst = mCapacity - mWrite;
            const size_t firstLen = std::min(bytesToWrite, maxFirst);

            out.first.ptr = mBuf.data() + mWrite;
            out.first.len = firstLen;

            const size_t remain = bytesToWrite - firstLen;
            if (remain > 0) {
                out.second.ptr = mBuf.data();
                out.second.len = std::min(remain, mRead - Guard);
            }
        }

        return out;
    }

    void commitWrite(size_t bytes) override {
        if (mAborted || bytes == 0)
            return;

        const size_t freeTotal = spaceUnlocked();
        bytes = std::min(bytes, freeTotal);

        mWrite += bytes;
        if (mWrite >= mCapacity)
            mWrite %= mCapacity;
    }

    // helper for tests
    size_t push(const uint8_t* data, size_t len) {
        size_t total = 0;
        while (total < len) {
            const auto spans = claimWriteSpans(len - total);
            if (spans.total() == 0)
                break;

            size_t w = 0;
            if (spans.first.len) {
                std::memcpy(spans.first.ptr, data + total, spans.first.len);
                w += spans.first.len;
            }
            if (spans.second.len) {
                std::memcpy(spans.second.ptr, data + total + w, spans.second.len);
                w += spans.second.len;
            }
            commitWrite(w);
            total += w;
        }
        return total;
    }

   private:
    static constexpr size_t Guard = 1;  // distinguish full vs empty

    size_t availableUnlocked() const {
        if (mWrite >= mRead)
            return mWrite - mRead;
        return (mCapacity - mRead) + mWrite;
    }

    size_t spaceUnlocked() const {
        // keep 1 byte free (Guard) so full/empty are distinguishable
        return (mCapacity - availableUnlocked()) - Guard;
    }

   private:
    size_t mCapacity;
    std::vector<uint8_t> mBuf;
    size_t mWrite;
    size_t mRead;
    bool mAborted;
};

}  // namespace common
