/**
 * @file AdcReader.hpp
 * @brief Implementation of the IAdcReader interface for ESP32 ADC oneshot mode.
 * * This file contains the AdcReader class which manages ADC peripheral initialization,
 * channel configuration, and data acquisition.
 */

#pragma once

#include <esp_adc/adc_oneshot.h>

#include <cstddef>
#include <cstdint>
#include <map>

#include "IAdcReader.hpp"
#include "Mutex.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware abstraction layer implementation classes.
 */
namespace adapters {

/**
 * @class AdcReader
 * @brief Concrete implementation of an ADC reader using ESP-IDF's oneshot unit API.
 * * This class handles the low-level interactions with the Analog-to-Digital Converter,
 * providing thread-safe access to raw ADC readings and GPIO-to-channel mapping.
 */
class AdcReader final : public IAdcReader {
   public:
    /**
     * @brief Constructs a new AdcReader object.
     * Initializes internal state but does not configure hardware.
     */
    AdcReader();

    /**
     * @brief Destroys the AdcReader object.
     * Ensures any allocated hardware resources or handles are properly released.
     */
    ~AdcReader() override;

    /**
     * @brief Initializes the ADC unit hardware.
     * @return true if the ADC unit was successfully initialized, false otherwise.
     */
    bool init() override;

    /**
     * @brief Configures a specific GPIO pin for ADC functionality.
     * * @param gpioNum The GPIO number to be configured as an ADC channel.
     * @return true if the channel was successfully mapped and configured, false otherwise.
     */
    bool setupChannel(const uint32_t gpioNum) override;

    /**
     * @brief Reads a burst of raw ADC samples from a specified GPIO.
     * * This method is thread-safe and will block until the requested number of samples are read.
     * * @param gpioNum The GPIO pin to read from.
     * @param buffer Pointer to the integer array where samples will be stored.
     * @param count The number of raw samples to read.
     * @return true if all samples were read successfully, false if the GPIO is unconfigured or a
     * hardware error occurred.
     */
    bool readRawBurst(const uint32_t gpioNum, int* buffer, const size_t count) override;

   private:
    /**
     * @struct ChannelInfo
     * @brief Internal mapping between a GPIO and its corresponding ADC channel.
     */
    struct ChannelInfo {
        adc_channel_t channel; /**< ESP-IDF specific ADC channel identifier. */
        bool valid = false; /**< Flag indicating if this channel has been correctly initialized. */
    };

    /** @brief Map to look up ADC channel information based on GPIO number. */
    std::map<uint32_t, ChannelInfo> mLookup;

    /** @brief Handle for the ESP32 ADC oneshot unit. */
    adc_oneshot_unit_handle_t mUnitHandle;

    /** @brief Internal state tracking if the init() method was successful. */
    bool mIsInitialized;

    /** @brief Mutex to ensure thread-safe access to the ADC hardware during burst reads. */
    common::Mutex mMutex;
};

}  // namespace adapters
