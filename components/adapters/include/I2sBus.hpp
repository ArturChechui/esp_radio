/**
 * @file I2sBus.hpp
 * @brief Implementation of the II2sBus interface for ESP32 I2S audio output.
 * * This file contains the I2sBus class which manages the initialization,
 * deinitialization, and data transmission for the I2S peripheral, typically
 * used for driving external DACs or amplifiers.
 */

#pragma once

#include <driver/i2s_std.h>

#include "II2sBus.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware abstraction layer implementation classes.
 */
namespace adapters {

/**
 * @class I2sBus
 * @brief Concrete implementation of an I2S bus controller for audio transmission.
 * * This class provides a high-level wrapper around the ESP-IDF I2S driver,
 * specifically configured for standard (STD) mode transmission. It allows
 * for dynamic sample rate reconfiguration and synchronous data writing.
 */
class I2sBus final : public II2sBus {
   public:
    /**
     * @brief Constructs a new I2sBus object.
     */
    I2sBus();

    /**
     * @brief Destroys the I2sBus object and ensures resources are released.
     */
    ~I2sBus() override;

    /**
     * @brief Initializes the I2S peripheral and allocates DMA resources.
     * @return true if the I2S channel was successfully initialized, false otherwise.
     */
    bool init() override;

    /**
     * @brief Deinitializes the I2S peripheral and frees associated handles.
     */
    void deinit() override;

    /**
     * @brief Writes audio data to the I2S bus.
     * * This is a blocking call that writes 16-bit PCM samples to the internal
     * DMA buffers.
     * * @param data Pointer to the buffer containing 16-bit audio samples.
     * @param size The number of bytes to write.
     * @param timeoutMs Maximum time to wait for the write operation to complete.
     * @return The number of bytes actually written to the DMA buffer.
     */
    size_t write(const int16_t* data, const size_t& size, const uint32_t& timeoutMs) override;

    /**
     * @brief Reconfigures the I2S clock to a new sample rate.
     * * This method allows changing the playback speed (e.g., from 44.1kHz to 48kHz)
     * without fully deinitializing the peripheral.
     * * @param sampleRate The new sample rate in Hz.
     * @return true if the clock was successfully updated, false otherwise.
     */
    bool reconfigureClock(const uint32_t& sampleRate) override;

    /**
     * @brief Retrieves the currently configured sample rate.
     * @return The current sample rate in Hz.
     */
    uint32_t getSampleRate() const override;

   private:
    /** @brief Handle for the ESP32 I2S transmit channel. */
    i2s_chan_handle_t mI2sTxHandle;

    /** @brief Internal tracking of the current sample rate configuration. */
    uint32_t mCurrentSampleRate;
};

}  // namespace adapters
