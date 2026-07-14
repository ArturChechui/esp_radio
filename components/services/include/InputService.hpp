/**
 * @file InputService.hpp
 * @brief Implementation of the IInputService interface for handling physical controls.
 *
 * This file contains the InputService class, which monitors GPIO inputs (buttons/encoders),
 * performs debouncing, and dispatches events to the system's core event queue.
 */

#pragma once

#include <array>
#include <cstdint>

#include "IInputService.hpp"
#include "Types.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware abstraction layer interfaces.
 */
namespace adapters {
class IGpioInput;
class IPersistentStorage;
}  // namespace adapters

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {
class IEventQueue;
template <typename T>
class IQueue;
class ITaskRunner;
class IClock;
}  // namespace common

/**
 * @namespace services
 * @brief Contains business logic service implementations.
 */
namespace services {

/**
 * @class InputService
 * @brief Concrete implementation of input management logic.
 *
 * InputService encapsulates the logic for reading digital inputs and rotary
 * encoders. It runs a periodic background task to poll GPIO levels, handle
 * software debouncing for buttons, and track quadrature states for encoders.
 * Detected interactions are packaged into AppEvents and sent to the core logic.
 */
class InputService : public IInputService {
   public:
    /**
     * @brief Constructs an InputService with its required hardware and system dependencies.
     * @param gpioInput Reference to the GPIO hardware adapter.
     * @param coreEventQueue Queue where application-level events (like ButtonPressed) are sent.
     * @param queue Generic messaging queue used for internal synchronization.
     * @param runner The task runner used to spawn the polling/processing thread.
     * @param clock Reference to the system clock for timestamping and debouncing.
     * @param persistentStorage Storage adapter for loading/saving input-related settings.
     */
    explicit InputService(adapters::IGpioInput& gpioInput, common::IEventQueue& coreEventQueue,
                          common::IQueue<uint32_t>& queue, common::ITaskRunner& runner,
                          common::IClock& clock, adapters::IPersistentStorage& persistentStorage);

    /** @brief Default virtual destructor. */
    ~InputService() override = default;

    /**
     * @brief Initializes GPIO pins and starts the background processing task.
     * @return true if initialization and task startup succeeded.
     */
    bool init() override;

    /**
     * @brief Stops the background task and releases hardware resources.
     */
    void deinit() override;

    /**
     * @brief Configures the service for day or night operation.
     * * This may adjust LED brightness (if applicable) or change the behavior of
     * light-sensitive input components.
     * @param night True to enable night-mode settings.
     */
    void setMode(const bool night) override;

   private:
    /**
     * @brief Static wrapper for the background task entry point.
     * @param arg Pointer to the InputService instance.
     * @param token Token used to check if the task should stop.
     * @return Result indicating if the task should continue or stop.
     */
    static common::StepResult processStepFn(void* arg, common::IStopToken& token);

    /**
     * @brief Internal logic executed on each iteration of the background task.
     * * This method polls all managed inputs and handles the main state machine
     * for input detection.
     * @param token Stop token for the background thread.
     * @return The result of the processing step.
     */
    common::StepResult processStep(common::IStopToken& token);

    /**
     * @brief Reads and processes the state of the rotary encoder GPIOs.
     * * Updates the quadrature state and detects clockwise/counter-clockwise movement.
     */
    void handleEncoderGpio();

    /**
     * @brief Translates physical encoder detents into volume level changes.
     * @param detents The number of steps (positive or negative) moved by the encoder.
     */
    void applyDetentsToVolume(const int detents);

    /**
     * @brief Handles the debouncing and event generation for a specific button GPIO.
     * @param gpioNum The GPIO pin number to check.
     * @param token Stop token to ensure responsiveness during shutdown.
     */
    void handleButtonGpio(const uint32_t gpioNum, common::IStopToken& token);

    adapters::IGpioInput& mGpioInput;     /**< Reference to GPIO hardware adapter. */
    common::IEventQueue& mCoreEventQueue; /**< Queue for dispatching AppEvents. */
    common::IQueue<uint32_t>& mQueue;     /**< Internal synchronization queue. */
    common::ITaskRunner& mTaskRunner;     /**< Reference to the background task manager. */
    common::TaskHandle mTaskHandle;       /**< Handle to the running input processing task. */
    common::IClock& mClock;               /**< Reference to system time. */
    adapters::IPersistentStorage& mPersistentStorage; /**< Reference to configuration storage. */

    std::array<uint64_t, 3> mLastPressMs; /**< Last detected press timestamps for debounce. */
    uint8_t mEncPrevQuadratureState;      /**< Previous state of the rotary encoder signals. */

    int mEncQuarterAccumulator; /**< Accumulates sub-detent steps (e.g., 1/4 pulses) to ensure
                                   volume changes only occur on physical clicks. */
    bool mEncInvert; /**< If true, reverses the logical direction (CW vs CCW) of the rotary encoder.
                      */
    uint32_t mVolume;    /**< The current system volume level. (0-100) */
    uint32_t mMaxVolume; /**< The upper limit for the volume level, used for clamping and scaling.
                            (0-100) */
};

}  // namespace services
