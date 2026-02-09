#pragma once

#include <string>

#include "Events.hpp"
#include "Types.hpp"

namespace commands {
class ICommand {
   public:
    virtual ~ICommand() = default;

    virtual bool handle(const common::AppEvent& e) = 0;
    virtual bool isFinished() = 0;
    virtual common::CommandType getCmdType() = 0;
};

}  // namespace commands
