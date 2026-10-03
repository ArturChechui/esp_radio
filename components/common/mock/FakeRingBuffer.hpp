#pragma once
#include <cstring>

#include "RingBuffer.hpp"

namespace common {

class FakeRingBuffer : public RingBuffer {
   public:
    explicit FakeRingBuffer(size_t capacityBytes) : RingBuffer(capacityBytes) {}

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
};

}  // namespace common
