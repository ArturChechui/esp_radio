/**
 * @file II2cBus.hpp
 * @brief Interface definition for I2C bus master operations.
 *
 * This file defines the abstract interface for an I2C bus, providing
 * standardized methods for basic data transmission and reception.
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
 * @class II2cBus
 * @brief Abstract interface for an I2C master bus.
 *
 * This interface allows the system to communicate with I2C slave devices
 * without being tied to a specific microcontroller's I2C peripheral implementation.
 */
class II2cBus {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~II2cBus() = default;

    /**
     * @brief Initializes the I2C master bus hardware.
     * @return true if the bus was successfully initialized, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Writes a sequence of bytes to a specific I2C slave device.
     * @param deviceAddr The 7-bit I2C address of the target slave device.
     * @param data Pointer to the buffer containing data to be sent.
     * @param len The number of bytes to transmit.
     * @param timeoutMs The maximum time to wait for the operation to complete.
     * @return true if the write operation was successful, false otherwise.
     */
    virtual bool writeBytes(const uint8_t deviceAddr, const uint8_t* data, const size_t len,
                            const uint32_t timeoutMs) = 0;

    /**
     * @brief Reads a sequence of bytes from a specific I2C slave device.
     * @param deviceAddr The 7-bit I2C address of the target slave device.
     * @param data Pointer to the buffer where the received data will be stored.
     * @param len The number of bytes to receive.
     * @param timeoutMs The maximum time to wait for the operation to complete.
     * @return true if the read operation was successful, false otherwise.
     */
    virtual bool readBytes(const uint8_t deviceAddr, uint8_t* data, const size_t len,
                           const uint32_t timeoutMs) = 0;
};

}  // namespace adapters
