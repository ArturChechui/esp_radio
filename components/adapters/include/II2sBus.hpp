/**
 * @file II2sBus.hpp
 * @brief Interface definition for I2S bus operations for audio output.
 *
 * This file defines the abstract interface for an I2S bus, providing
 * standardized methods for audio data transmission and clock management.
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @namespace adapters
 * @brief Contains hardware abstraction layer implementation and interface classes.
 */
namespace adapters {

/**
 * @class II2sBus
 * @brief Abstract interface for an I2S bus controller.
 *
 * This interface encapsulates the hardware-specific details of I2S initialization,
 * clock configuration, and PCM data writing, allowing the system to output audio
 * regardless of the specific DAC or MCU peripheral being used.
 */
class II2sBus {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~II2sBus() = default;

    /**
     * @brief Initializes the I2S bus and associated hardware.
     * @return true if the I2S peripheral was successfully initialized, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Deinitializes the I2S bus and releases hardware resources.
     */
    virtual void deinit() = 0;

    /**
     * @brief Writes PCM audio data to the I2S output.
     * * @param data Pointer to the buffer containing 16-bit PCM samples (typically interleaved).
     * @param size The number of bytes to transmit.
     * @param timeoutMs The maximum time to wait for the write operation to complete.
     * @return The number of bytes actually written to the output buffer/DMA.
     */
    virtual size_t write(const int16_t* data, const size_t& size, const uint32_t& timeoutMs) = 0;

    /**
     * @brief Reconfigures the I2S clock to support a different audio sample rate.
     * @param sampleRate The target sample rate in Hz (e.g., 44100, 48000).
     * @return true if the clock was successfully reconfigured, false otherwise.
     */
    virtual bool reconfigureClock(const uint32_t& sampleRate) = 0;

    /**
     * @brief Retrieves the currently configured sample rate of the bus.
     * @return The sample rate in Hz.
     */
    virtual uint32_t getSampleRate() const = 0;
};

}  // namespace adapters
