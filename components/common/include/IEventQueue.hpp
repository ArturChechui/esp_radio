/**
 * @file IEventQueue.hpp
 * @brief Interface for the system-wide event distribution system.
 *
 * This file defines the IEventQueue interface, which serves as the primary
 * mechanism for services to broadcast state changes and hardware events.
 */

#pragma once

#include "Events.hpp"

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class IEventQueue
 * @brief Abstract interface for an asynchronous message queue.
 *
 * This interface decouples event producers (like sensors or buttons) from
 * the event consumer (the main application logic). Components hold a
 * reference to this interface to "fire and forget" events without
 * knowing how they are processed.
 */
class IEventQueue {
   public:
    /** @brief Virtual destructor for safe interface cleanup. */
    virtual ~IEventQueue() = default;

    /**
     * @brief Pushes an application event into the processing queue.
     * * Depending on the implementation, this may be a thread-safe operation
     * that allows different tasks to communicate asynchronously.
     * * @param event The application event (variant) to be queued.
     * @return true if the event was successfully accepted by the queue.
     */
    virtual bool post(const AppEvent& event) = 0;
};

}  // namespace common
