/**
 * @file IGpioInput.hpp
 * @brief Interface definition for General Purpose Input/Output (GPIO) input operations.
 *
 * This file defines the abstract interface for GPIO input drivers, providing
 * standardized methods for initialization and state reading.
 */

#pragma once

/**
 * @namespace adapters
 * @brief Contains hardware abstraction layer implementation and interface classes.
 */
namespace adapters {

/**
 * @class IGpioInput
 * @brief Abstract interface for a GPIO input controller.
 *
 * This interface defines the contract for implementing GPIO input functionality,
 * such as reading digital levels and managing the lifecycle of the GPIO peripheral.
 */
class IGpioInput {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IGpioInput() = default;

    /**
     * @brief Initializes the GPIO peripheral or specific pins.
     * @return true if the initialization was successful, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Deinitializes the GPIO peripheral and releases hardware resources.
     */
    virtual void deinit() = 0;

    /**
     * @brief Reads the current logic level of a specified GPIO pin.
     * @param gpioNum The numeric identifier of the GPIO pin.
     * @return The logic level (typically 0 or 1), or a negative value on error.
     */
    virtual int getLevel(const uint32_t gpioNum) = 0;
};
}  // namespace adapters
