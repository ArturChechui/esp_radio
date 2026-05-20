/**
 * @file IDisplay.hpp
 * @brief Interface definition for display peripherals.
 *
 * This file defines the abstract interface for display drivers, providing
 * standardized methods for initialization and data rendering.
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
 * @class IDisplay
 * @brief Abstract interface for controlling a display device.
 *
 * This interface defines the basic operations required to drive a display,
 * including initialization and various methods to push pixel data to the screen.
 */
class IDisplay {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IDisplay() = default;

    /**
     * @brief Initializes the display hardware and controller.
     * @return true if initialization was successful, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Performs a full framebuffer write to the display.
     * * The data is expected to follow a page-major layout where pages are
     * processed first, followed by columns.
     * * @param data Pointer to the framebuffer pixel data.
     * @param len The size of the data buffer in bytes.
     * @return true if the operation succeeded, false otherwise.
     */
    virtual bool showFramebuffer(const uint8_t* data, const size_t len) = 0;

    /**
     * @brief Performs a windowed write using specific column and page addressing.
     * * This method allows updating only a specific portion of the display.
     * The data layout follows: for each page in [page0..page1], and for each
     * column in [col0..col1], push the corresponding byte.
     * * @param col0 Starting column address.
     * @param col1 Ending column address.
     * @param page0 Starting page (row group) address.
     * @param page1 Ending page (row group) address.
     * @param data Pointer to the pixel data for the window.
     * @param len The size of the window data in bytes.
     * @return true if the window update succeeded, false otherwise.
     */
    virtual bool showWindow(const uint8_t col0, const uint8_t col1, const uint8_t page0,
                            const uint8_t page1, const uint8_t* data, const size_t len) = 0;
};

}  // namespace adapters
