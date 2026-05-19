/**
 * @file ISensorService.hpp
 * @brief Interface definition for the sensor management service.
 *
 * This file defines the abstract interface for a service responsible for
 * initializing and managing hardware sensors, including audio input control.
 */

#pragma once

/**
 * @namespace services
 * @brief Contains business logic services that coordinate hardware and application state.
 */
namespace services {

/**
 * @class ISensorService
 * @brief Abstract interface for a sensor coordination service.
 *
 * This service provides a standardized way to interact with various sensors
 * (e.g., light sensors, temperature sensors) and control the activation state
 * of audio input hardware like microphones.
 */
class ISensorService {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~ISensorService() = default;

    /**
     * @brief Initializes the sensor service and any associated hardware drivers.
     * @return true if the sensor subsystem was successfully initialized, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Deinitializes the sensor service and releases hardware resources.
     */
    virtual void deinit() = 0;

    /**
     * @brief Enables or disables the microphone or audio input path.
     * * This can be used to manage power consumption or ensure privacy when
     * voice-related features are not in use.
     * @param active Set to true to power on/enable the microphone, false to disable.
     */
    virtual void startClapDetection(const bool active) = 0;

    /**
     * @brief Toggles the persistent master state of the clap detection feature.
     * @details When disabled, all calls to startClapDetection(true) are ignored.
     * This state is saved to persistent storage.
     * @return True if the feature is now enabled, false if it is now disabled.
     */
    virtual bool toggleClapFeature() = 0;
};
}  // namespace services
