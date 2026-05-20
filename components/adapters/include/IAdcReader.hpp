/**
 * @file IAdcReader.hpp
 * @brief Interface definition for an Analog-to-Digital Converter (ADC) reader.
 *
 * This file defines the abstract interface for hardware-specific ADC implementations,
 * allowing for standardized interaction with ADC peripherals.
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
 * @class IAdcReader
 * @brief Abstract interface for reading analog signals from hardware.
 *
 * This interface provides the necessary methods to initialize an ADC unit,
 * configure specific GPIO pins as ADC channels, and perform raw data acquisition.
 */
class IAdcReader {
   public:
    /** @brief The maximum measurable voltage of the ADC in millivolts (3300mV). */
    static constexpr uint16_t MaxAdcMv = 3300U;

    /** @brief The maximum raw value for a 12-bit ADC reading (4095). */
    static constexpr uint16_t MaxAdcRaw = 4095U;

    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IAdcReader() = default;

    /**
     * @brief Initializes the ADC hardware unit.
     * @return true if initialization was successful, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Configures a specific GPIO pin to act as an ADC input channel.
     * @param gpioNum The GPIO number to be configured.
     * @return true if the channel setup was successful, false otherwise.
     */
    virtual bool setupChannel(const uint32_t gpioNum) = 0;

    /**
     * @brief Reads a specific number of raw samples from the configured GPIO.
     * * @param gpioNum The GPIO pin to read samples from.
     * @param buffer Pointer to an integer array to store the raw ADC values.
     * @param count The number of raw samples to be acquired.
     * @return true if the burst read operation completed successfully, false otherwise.
     */
    virtual bool readRawBurst(const uint32_t gpioNum, int* buffer, const size_t count) = 0;
};
}  // namespace adapters
