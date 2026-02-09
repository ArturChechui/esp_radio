#pragma once

#include "Events.hpp"

namespace common {

/**
 * @brief Event handler interface
 * Any class can handle events by implementing this
 */
class IEventHandler {
   public:
    virtual ~IEventHandler() = default;

    /**
     * @brief Handle an event
     * @param event The event to process
     */
    virtual void onEvent(const AppEvent& event) = 0;
};

}  // namespace common
