/**
 * @file ICommand.hpp
 * @brief Interface definition for the Command design pattern.
 *
 * This file defines the abstract interface for commands, allowing the
 * AppController to execute and monitor complex application logic
 * asynchronously.
 */

#pragma once

#include <optional>
#include <string>

#include "Events.hpp"
#include "Types.hpp"

/**
 * @namespace core::commands
 * @brief Contains the command pattern infrastructure and concrete implementations.
 */
namespace core::commands {

/**
 * @class ICommand
 * @brief Abstract interface for an executable command.
 *
 * Commands encapsulate a specific piece of business logic (like connecting to Wi-Fi
 * or updating a station list). The interface allows the controller to feed events
 * into the command and check when the operation has reached a terminal state.
 */
class ICommand {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived command classes.
     */
    virtual ~ICommand() = default;

    /**
     * @brief Processes an application event within the context of the command.
     * * Derived classes implement this to react to state changes, such as
     * timer expirations or network status updates, to progress their internal logic.
     * * @param e The application event to be handled by the command.
     */
    virtual void handle(const common::AppEvent& e) = 0;

    /**
     * @brief Checks if the command has finished its execution lifecycle.
     * * The controller uses this to determine when it can safely destroy the
     * command object and transition to a new state or process the next command.
     * * @return true if the command is complete, false if it is still active.
     */
    virtual bool isFinished() = 0;

    /**
     * @brief Retrieves the type of the command for identification and routing purposes.
     * @return The type of the command.
     */
    virtual common::CommandType getCmdType() = 0;

    /**
     * @brief Retrieves the result of the command's execution.
     * @return An optional boolean indicating success (true), failure (false), or nullopt if the
     * result is not yet available since the command is still executing.
     */
    virtual std::optional<bool> getResult() = 0;
};
}  // namespace core::commands
