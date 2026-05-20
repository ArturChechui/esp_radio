/**
 * @file IMp3Decoder.hpp
 * @brief Interface definition for MP3 audio decoding.
 *
 * This file defines the abstract interface for MP3 decoders, providing
 * constants for frame sizes and methods for bitstream processing.
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @namespace common
 * @brief Contains shared data structures used across the application.
 */
namespace common {
/** @brief Forward declaration of the structure containing decoded frame metadata. */
struct Mp3FrameInfo;
}  // namespace common

/**
 * @namespace adapters
 * @brief Contains hardware and protocol abstraction layer implementation and interface classes.
 */
namespace adapters {

/** * @brief Maximum number of PCM samples per decoded MP3 frame.
 * * This value corresponds to the MINIMP3_MAX_SAMPLES_PER_FRAME (1152 samples per channel).
 */
static constexpr uint32_t MaxSamplesPerFrame = 1152U * 2U;

/** @brief Maximum size in bytes for a single decoded PCM frame. */
static constexpr uint32_t MaxBytesPerFrame = MaxSamplesPerFrame * sizeof(int16_t);

/**
 * @class IMp3Decoder
 * @brief Abstract interface for an MP3 decoder.
 *
 * This interface allows the system to decode MP3 bitstreams into 16-bit PCM samples.
 * It is designed to work frame-by-frame, providing feedback on how much data was
 * consumed and how many samples were generated.
 */
class IMp3Decoder {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IMp3Decoder() = default;

    /**
     * @brief Decodes a single frame or chunk of MP3 data into PCM.
     * @param data Pointer to the input MP3 bitstream data.
     * @param len Length of the input data in bytes.
     * @param pcmOut Pointer to the buffer where decoded 16-bit PCM samples will be stored.
     * @param pcmSamplesCap The capacity of the pcmOut buffer in terms of samples (int16_t).
     * @return common::Mp3FrameInfo Struct containing the number of bytes consumed from 'data'
     * and the number of PCM samples produced.
     */
    virtual common::Mp3FrameInfo decode(const uint8_t* data, size_t len, int16_t* pcmOut,
                                        size_t pcmSamplesCap) = 0;

    /**
     * @brief Resets the internal state of the decoder.
     * This should be called when switching streams or after a seek operation
     * to clear any buffered data or internal bitstream state.
     */
    virtual void reset() = 0;
};

}  // namespace adapters
