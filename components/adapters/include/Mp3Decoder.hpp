/**
 * @file Mp3Decoder.hpp
 * @brief Implementation of the IMp3Decoder interface using the minimp3 library.
 *
 * This file contains the Mp3Decoder class, which wraps the minimp3 decoder
 * to provide frame-by-frame MP3 bitstream decoding into PCM audio.
 */

#pragma once

#include <cstdint>

#include "IMp3Decoder.hpp"
#include "minimp3.h"

/**
 * @namespace adapters
 * @brief Contains hardware and protocol abstraction layer implementation classes.
 */
namespace adapters {

/**
 * @class Mp3Decoder
 * @brief Concrete MP3 decoder implementation using the minimp3 library.
 *
 * This class manages the internal state of a minimp3 decoder instance. It is
 * used to convert compressed MP3 data chunks into raw 16-bit PCM samples
 * suitable for playback via I2S or other audio interfaces.
 */
class Mp3Decoder : public IMp3Decoder {
   public:
    /**
     * @brief Constructs a new Mp3Decoder object and initializes the minimp3 state.
     */
    Mp3Decoder();

    /**
     * @brief Default destructor.
     */
    ~Mp3Decoder() override = default;

    /**
     * @brief Decodes a chunk of MP3 data into PCM samples.
     * * Uses the internal minimp3 state to process the bitstream. It returns
     * information about how many bytes were consumed and how many samples
     * were generated.
     * @param data Pointer to the input MP3 bitstream.
     * @param len Length of the input data in bytes.
     * @param pcmOut Pointer to the output buffer for 16-bit PCM samples.
     * @param pcmSamplesCap The maximum number of samples the output buffer can hold.
     * @return common::Mp3FrameInfo Struct containing 'bytesConsumed' and 'samplesDecoded'.
     */
    common::Mp3FrameInfo decode(const uint8_t* data, size_t len, int16_t* pcmOut,
                                size_t pcmSamplesCap) override;

    /**
     * @brief Resets the internal minimp3 decoder state.
     * Clears internal buffers and bitstream synchronization state, effectively
     * preparing the decoder for a new stream or a seek operation.
     */
    void reset() override;

   private:
    /** @brief Internal state structure for the minimp3 decoder. */
    mp3dec_t mDec;
};

}  // namespace adapters
