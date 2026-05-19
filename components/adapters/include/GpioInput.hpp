/**
 * @file GpioInput.hpp
 * @brief Implementation of the IGpioInput interface for managing GPIO hardware.
 * * This file contains the GpioInput class, which handles GPIO initialization,
 * interrupt configuration, and input level reading.
 */

#pragma once

#include <cstdint>

#include "IGpioInput.hpp"

/**
 * @namespace common
 * @brief Contains shared utility components like data structures and synchronization primitives.
 */
namespace common {
/**
 * @brief Forward declaration of the Queue template used for event processing.
 * @tparam T The type of data stored in the queue.
 */
template <typename T>
class Queue;
}  // namespace common

/**
 * @namespace adapters
 * @brief Contains hardware abstraction layer implementation classes.
 */
namespace adapters {

/**
 * @class GpioInput
 * @brief Concrete implementation of a GPIO input controller.
 * * This class manages GPIO pins configured as inputs. It supports interrupt-driven
 * event handling by pushing GPIO numbers to a shared queue when state changes occur.
 * It follows a singleton-like pattern for ISR access via a static instance pointer.
 */
class GpioInput : public IGpioInput {
   public:
    /**
     * @brief Constructs a GpioInput object.
     * @param queue A reference to a thread-safe queue where GPIO interrupt events are posted.
     */
    GpioInput(common::Queue<uint32_t>& queue);

    /**
     * @brief Destructor. Ensures that the static instance pointer is cleared and resources are
     * released.
     */
    ~GpioInput() override;

    /**
     * @brief Initializes the GPIO peripheral and installs the global ISR service.
     * @return true if the GPIO service was successfully initialized, false otherwise.
     */
    bool init() override;

    /**
     * @brief Deinitializes the GPIO service and removes installed handlers.
     */
    void deinit() override;

    /**
     * @brief Reads the current digital level of a specific GPIO pin.
     * @param gpioNum The GPIO number to query.
     * @return The logic level (0 or 1), or a negative value on error.
     */
    int getLevel(const uint32_t gpioNum) override;

   private:
    /**
     * @brief Static Interrupt Service Routine (ISR) handler.
     * * This function is called by the hardware when a GPIO interrupt occurs.
     * It dispatches the event to the queue associated with the GpioInput instance.
     * @param arg Pointer to the GpioInput instance (or specific context).
     */
    static void gpioIsrHandler(void* arg);

    /** @brief Static pointer to the active GpioInput instance, used by the ISR to access member
     * variables. */
    static GpioInput* sInstance;

    /** @brief Reference to the queue used for notifying the system of GPIO state changes. */
    common::Queue<uint32_t>& mQueue;

    /** @brief Internal flag tracking whether the GPIO service is currently initialized. */
    bool mIsInitialized;
};
}  // namespace adapters
