#pragma once

#include <cstdint>

#include "IMp3Decoder.hpp"
#include "minimp3.h"

namespace adapters {
class Mp3Decoder : public IMp3Decoder {
   public:
    Mp3Decoder();
    ~Mp3Decoder() override = default;
    common::Mp3FrameInfo decode(const uint8_t* data, size_t len, int16_t* pcmOut,
                                size_t pcmSamplesCap) override;

    void reset() override;

   private:
    mp3dec_t mDec{};
};

}  // namespace adapters
