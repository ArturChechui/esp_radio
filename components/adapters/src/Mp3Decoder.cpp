#include "Mp3Decoder.hpp"

#include <cstring>

#include "minimp3.h"
#include "Types.hpp"

namespace adapters {
Mp3Decoder::Mp3Decoder() {
    reset();
}

common::Mp3FrameInfo Mp3Decoder::decode(const uint8_t* data, size_t len, int16_t* pcmOut,
                                        size_t pcmSamplesCap) {
    common::Mp3FrameInfo out{};

    if (!data || len < 4UL || !pcmOut || pcmSamplesCap == 0UL ||
        pcmSamplesCap < MINIMP3_MAX_SAMPLES_PER_FRAME) {
        return out;
    }

    mp3dec_frame_info_t info{};
    const int inBytes = (len > static_cast<size_t>(INT32_MAX)) ? INT32_MAX : static_cast<int>(len);
    const int samplesPerCh = mp3dec_decode_frame(&mDec, data, inBytes, pcmOut, &info);

    out.frameBytes = info.frame_bytes;
    out.hz = info.hz;
    out.channels = info.channels;
    out.samplesPerCh = samplesPerCh;

    return out;
}

void Mp3Decoder::reset() {
    std::memset(&mDec, 0, sizeof(mDec));
    mp3dec_init(&mDec);
}

}  // namespace adapters