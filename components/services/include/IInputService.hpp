/**
 * @file IInputService.hpp
 * @brief Interface definition for the input management service.
 *
 * This file defines the abstract interface for a service that processes
 * physical inputs and manages input-related configurations like night mode.
 */

#pragma once

#include <cstdint>

/**
 * @namespace services
 * @brief Contains business logic services that coordinate hardware and application state.
 */
namespace services {

/** * @brief Threshold in Lux below which the system is considered to be in 'night' conditions.
 */
constexpr uint16_t NightLux = 5U;

/**
 * @class IInputService
 * @brief Abstract interface for an input processing service.
 *
 * This service is responsible for initializing input hardware, monitoring
 * user interactions, and adjusting input sensitivity or behavior based on
 * environmental modes (e.g., dimming lights or changing button debounce).
 */
class IInputService {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IInputService() = default;

    /**
     * @brief Initializes the input service and its underlying hardware drivers.
     * @return true if the input system was successfully set up, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Deinitializes the input service and releases associated resources.
     */
    virtual void deinit() = 0;

    /**
     * @brief Sets the operational mode of the input service based on lighting conditions.
     * * @param night Set to true to enable night mode optimizations (e.g., lower brightness),
     * false for standard daylight operation.
     */
    virtual void setMode(const bool night) = 0;
};

}  // namespace services
