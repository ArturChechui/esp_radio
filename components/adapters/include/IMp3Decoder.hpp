#pragma once

#include <cstddef>
#include <cstdint>

namespace common {
struct Mp3FrameInfo;
}  // namespace common

namespace adapters {
// Has to be same as MINIMP3_MAX_SAMPLES_PER_FRAME
static constexpr uint32_t MaxSamplesPerFrame = 1152U * 2U;
static constexpr uint32_t MaxBytesPerFrame = MaxSamplesPerFrame * sizeof(int16_t);

class IMp3Decoder {
   public:
    virtual ~IMp3Decoder() = default;
    virtual common::Mp3FrameInfo decode(const uint8_t* data, size_t len, int16_t* pcmOut,
                                        size_t pcmSamplesCap) = 0;
    virtual void reset() = 0;
};

}  // namespace adapters
