#pragma once

#include <optional>
#include <string>

#include "Events.hpp"
#include "ICommand.hpp"
#include "Types.hpp"

namespace common {
class IEventQueue;
}  // namespace common

namespace services {
class IPlayerService;
class IStationRepository;
}  // namespace services

namespace core::commands {

class PlayStopSkipCommand : public ICommand {
   public:
    enum class Action { Play, Stop, SkipNext, SkipPrevious };

    PlayStopSkipCommand(services::IPlayerService& playerService,
                        services::IStationRepository& stationRepo,
                        common::IEventQueue& uiEventQueue, const common::Button btn);
    ~PlayStopSkipCommand() override = default;

    void handle(const common::AppEvent& e) override;
    bool isFinished() override;

   private:
    void startAction();  // does the initial play/stop/skip call
    void onPlaybackStatus(common::PlaybackStatus s);

   private:
    services::IPlayerService& mPlayerService;
    services::IStationRepository& mStationRepo;
    common::IEventQueue& mUiEventQueue;
    common::Button mButton;
    Action mRequestedAction;
    bool mStarted;
    bool mFinished;
};

}  // namespace core::commands
