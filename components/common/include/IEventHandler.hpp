/**
 * @file IEventHandler.hpp
 * @brief Interface for application event handling.
 *
 * This file defines the IEventHandler interface, which is implemented by
 * any component that needs to react to system-wide AppEvents.
 */

#pragma once

#include "Events.hpp"

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class IEventHandler
 * @brief Abstract interface for processing application events.
 *
 * Classes that implement this interface can be registered with the
 * event system (e.g., EventTask) to receive and handle notifications
 * about state changes, hardware inputs, and system events.
 */
class IEventHandler {
   public:
    /** @brief Virtual destructor for safe interface cleanup. */
    virtual ~IEventHandler() = default;

    /**
     * @brief Method called by the event system when a new event is dispatched.
     * @param event The application event to process.
     */
    virtual void onEvent(const AppEvent& event) = 0;
};

}  // namespace common
