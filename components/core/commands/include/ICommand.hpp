#pragma once

#include <string>

#include "Events.hpp"
#include "Types.hpp"

namespace core::commands {
class ICommand {
   public:
    virtual ~ICommand() = default;

    virtual void handle(const common::AppEvent& e) = 0;
    virtual bool isFinished() = 0;
};
}  // namespace core::commands
