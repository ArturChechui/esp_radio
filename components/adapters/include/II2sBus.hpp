#pragma once

#include <cstddef>
#include <cstdint>

namespace adapters {

/**
 * @brief I2S bus abstraction for audio output
 *
 * Encapsulates I2S hardware initialization, clock configuration, and data writing.
 */
class II2sBus {
   public:
    virtual ~II2sBus() = default;

    /**
     * @brief Initialize I2S bus
     * @return true if successful
     */
    virtual bool init() = 0;

    /**
     * @brief Deinitialize I2S bus and free resources
     */
    virtual void deinit() = 0;

    /**
     * @brief Write PCM data to I2S output
     * @param data Pointer to PCM samples (int16_t, interleaved)
     * @param size Number of bytes to write
     * @param timeoutMs Timeout in milliseconds
     * @return Number of bytes actually written
     */
    virtual size_t write(const int16_t* data, const size_t& size, const uint32_t& timeoutMs) = 0;

    /**
     * @brief Reconfigure I2S clock for a different sample rate
     * @param sampleRate Sample rate in Hz (e.g., 44100, 48000)
     * @return true if successful
     */
    virtual bool reconfigureClock(const uint32_t& sampleRate) = 0;

    /**
     * @brief Get current I2S sample rate
     */
    virtual uint32_t getSampleRate() const = 0;
};

}  // namespace adapters
