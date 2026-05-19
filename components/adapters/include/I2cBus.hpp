/**
 * @file I2cBus.hpp
 * @brief Implementation of the II2cBus interface for ESP32 I2C master communication.
 * * This file contains the I2cBus class, which manages the initialization of the
 * I2C master bus and provides methods for reading from and writing to I2C slave devices.
 */

#pragma once

#include <driver/i2c_master.h>

#include <cstddef>
#include <cstdint>
#include <map>

#include "II2cBus.hpp"
#include "Mutex.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware abstraction layer implementation classes.
 */
namespace adapters {

/**
 * @class I2cBus
 * @brief Concrete implementation of an I2C master bus controller.
 * * This class handles the low-level I2C master configuration using the ESP-IDF driver.
 * It maintains a cache of device handles to optimize communication with multiple
 * slaves on the same bus and ensures thread safety through a mutex.
 */
class I2cBus final : public II2cBus {
   public:
    /**
     * @brief Constructs a new I2cBus object.
     */
    I2cBus();

    /**
     * @brief Destroys the I2cBus object and releases bus resources.
     */
    ~I2cBus() override;

    /**
     * @brief Initializes the I2C master bus hardware.
     * @return true if the bus was successfully initialized, false otherwise.
     */
    bool init() override;

    /**
     * @brief Writes a sequence of bytes to a specific I2C slave device.
     * @param deviceAddr The 7-bit I2C address of the slave device.
     * @param data Pointer to the buffer containing data to write.
     * @param len Number of bytes to write.
     * @param timeoutMs Maximum time to wait for the operation to complete.
     * @return true if the write operation succeeded, false otherwise.
     */
    bool writeBytes(const uint8_t deviceAddr, const uint8_t *data, const size_t len,
                    const uint32_t timeoutMs) override;

    /**
     * @brief Reads a sequence of bytes from a specific I2C slave device.
     * @param deviceAddr The 7-bit I2C address of the slave device.
     * @param data Pointer to the buffer where received data will be stored.
     * @param len Number of bytes to read.
     * @param timeoutMs Maximum time to wait for the operation to complete.
     * @return true if the read operation succeeded, false otherwise.
     */
    bool readBytes(const uint8_t deviceAddr, uint8_t *data, const size_t len,
                   const uint32_t timeoutMs) override;

   private:
    /**
     * @brief Retrieves an existing device handle or creates a new one for a specific address.
     * * This internal helper manages the mDeviceHandles map to ensure each slave
     * device is correctly registered with the ESP-IDF I2C master driver.
     * * @param deviceAddr The I2C address of the target slave.
     * @return The handle for the I2C slave device, or nullptr on failure.
     */
    i2c_master_dev_handle_t getOrCreateDeviceHandle(const uint8_t deviceAddr);

    /** @brief Handle for the ESP32 I2C master bus. */
    i2c_master_bus_handle_t mBusHandle;

    /** @brief The configured frequency of the I2C bus in Hz. */
    uint32_t mFreqHz;

    /** @brief Map of I2C addresses to their corresponding ESP-IDF device handles. */
    std::map<uint8_t, i2c_master_dev_handle_t> mDeviceHandles;

    /** @brief Mutex to ensure thread-safe access to the I2C bus and device handles. */
    common::Mutex mMutex;
};

}  // namespace adapters
