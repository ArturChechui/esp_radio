/**
 * @file PlayStopSkipCommand.hpp
 * @brief Command for handling playback controls (Play, Stop, Skip).
 *
 * This file contains the PlayStopSkipCommand class, which translates button
 * presses into playback actions and coordinates updates across services.
 */

#pragma once

#include <optional>
#include <string>

#include "Events.hpp"
#include "ICommand.hpp"
#include "Types.hpp"

/**
 * @namespace common
 * @brief Contains shared messaging and event structures.
 */
namespace common {
class IEventQueue;
}  // namespace common

/**
 * @namespace services
 * @brief Contains business logic service interfaces.
 */
namespace services {
class IPlayerService;
class IStationRepository;
}  // namespace services

/**
 * @namespace core::commands
 * @brief Contains concrete command implementations for the application state machine.
 */
namespace core::commands {

/**
 * @class PlayStopSkipCommand
 * @brief Command that processes Play, Stop, and Skip logic.
 *
 * This command is triggered by physical button events. It determines the
 * appropriate action (e.g., if playing, stop; if stopped, play) and
 * communicates with the PlayerService to execute the change. It also handles
 * station skipping by interacting with the StationRepository.
 */
class PlayStopSkipCommand : public ICommand {
   public:
    /**
     * @brief Defines the basic playback actions supported by this command.
     */
    enum class Action {
        Play, /**< Command to start or resume playback. */
        Stop  /**< Command to halt playback. */
    };

    /**
     * @brief Constructs a PlayStopSkipCommand.
     * @param playerService Reference to the service controlling audio output.
     * @param stationRepo Reference to the repository containing station URLs.
     * @param uiEventQueue Queue for sending feedback events to the UI layer.
     * @param btn The button that triggered this command instance.
     */
    PlayStopSkipCommand(services::IPlayerService& playerService,
                        services::IStationRepository& stationRepo,
                        common::IEventQueue& uiEventQueue, const common::Button btn);

    /** @brief Default virtual destructor. */
    ~PlayStopSkipCommand() override = default;

    /**
     * @brief Processes events relevant to the playback action lifecycle.
     * @param e The application event to handle.
     */
    void handle(const common::AppEvent& e) override;

    /**
     * @brief Checks if the playback action sequence is complete.
     * @return true if the command has finished its work.
     */
    bool isFinished() override;

   private:
    /**
     * @brief Performs the initial logic for the play/stop/skip call.
     * * This determines whether to advance the station index or toggle the
     * current playback state based on the current system status.
     */
    void startAction();

    /**
     * @brief Internal handler for playback status update events.
     * @param s The new playback status (e.g., Playing, Stopped, Buffering).
     */
    void onPlaybackStatus(common::PlaybackStatus s);

   private:
    services::IPlayerService& mPlayerService;   /**< Reference to audio playback logic. */
    services::IStationRepository& mStationRepo; /**< Reference to station data storage. */

    common::IEventQueue& mUiEventQueue; /**< Queue for UI-bound notification events. */
    common::Button mButton;             /**< The button identifier that initiated the command. */
    Action mRequestedAction;            /**< The specific action being executed. */
    bool mStarted;                      /**< Flag: indicates if the action has been initiated. */
    bool mFinished; /**< Flag: indicates if the command is ready to be destroyed. */
};

}  // namespace core::commands
