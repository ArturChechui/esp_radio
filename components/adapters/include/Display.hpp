/**
 * @file Display.hpp
 * @brief Implementation of the IDisplay interface for I2C-based displays.
 * * This file contains the Display class which handles initialization and
 * data transmission to a display peripheral over an I2C bus.
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "IDisplay.hpp"
#include "II2cBus.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware abstraction layer implementation classes.
 */
namespace adapters {

/**
 * @class Display
 * @brief Concrete implementation of a display controller using I2C communication.
 * * This class provides methods to initialize the display hardware and update
 * the screen content using full framebuffers or specific windowed regions.
 */
class Display final : public IDisplay {
   public:
    /**
     * @brief Constructs a new Display object.
     * @param i2cBus Reference to the I2C bus interface used for communication.
     */
    explicit Display(II2cBus& i2cBus);

    /**
     * @brief Default destructor.
     */
    ~Display() override = default;

    /** @brief Deleted copy constructor to prevent unintended copying. */
    Display(const Display&) = delete;
    /** @brief Deleted assignment operator to prevent unintended copying. */
    Display& operator=(const Display&) = delete;

    /**
     * @brief Initializes the display hardware.
     * * This typically involves sending a sequence of startup commands over I2C.
     * @return true if initialization was successful, false otherwise.
     */
    bool init() override;

    /**
     * @brief Updates the entire display area with the provided framebuffer.
     * @param framebuffer Pointer to the buffer containing pixel data.
     * @param len The size of the framebuffer in bytes.
     * @return true if the data was successfully sent, false otherwise.
     */
    bool showFramebuffer(const uint8_t* framebuffer, const size_t len) override;

    /**
     * @brief Updates a specific rectangular region (window) on the display.
     * @param col0 Starting column address.
     * @param col1 Ending column address.
     * @param page0 Starting page (row group) address.
     * @param page1 Ending page (row group) address.
     * @param data Pointer to the pixel data for the window.
     * @param len The size of the data in bytes.
     * @return true if the window update was successful, false otherwise.
     */
    bool showWindow(const uint8_t col0, const uint8_t col1, const uint8_t page0,
                    const uint8_t page1, const uint8_t* data, const size_t len) override;

   private:
    /**
     * @brief Sends command bytes to the display controller.
     * @param cmd Pointer to the command byte(s).
     * @param len Number of command bytes to send.
     * @return true if the I2C transfer succeeded, false otherwise.
     */
    bool writeCommand(const uint8_t* cmd, const size_t len);

    /**
     * @brief Sends pixel or display data bytes to the display controller.
     * @param data Pointer to the data byte(s).
     * @param len Number of data bytes to send.
     * @return true if the I2C transfer succeeded, false otherwise.
     */
    bool writeData(const uint8_t* data, const size_t len);

   private:
    /** @brief Reference to the I2C bus used for communication. */
    II2cBus& mI2cBus;

    /** @brief The I2C slave address of the display device. */
    const uint8_t mI2cAddr;

    /** @brief Internal flag indicating if the display is initialized and ready. */
    bool mReady{false};
};

}  // namespace adapters
