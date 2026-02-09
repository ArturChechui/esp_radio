#pragma once

#include "Events.hpp"

namespace common {
/**
 * @brief Event queue interface
 * Services use this to post events
 */
class IEventQueue {
   public:
    virtual ~IEventQueue() = default;

    /**
     * @brief Post an event to the queue
     */
    virtual bool post(const AppEvent& event) = 0;
};

}  // namespace common
