#pragma once

#include <optional>
#include <string>

#include "Events.hpp"
#include "ICommand.hpp"
#include "Types.hpp"

namespace services {
class IPlayerService;
class IStationRepository;
}  // namespace services

namespace commands {

class PlayStopSkipCommand : public ICommand {
   public:
    enum class Action { TogglePlayStop, SkipNext, SkipPrevious };

    PlayStopSkipCommand(services::IPlayerService& playerService,
                        services::IStationRepository& stationRepo, const common::Button btn);
    ~PlayStopSkipCommand() override = default;

    bool handle(const common::AppEvent& e) override;
    bool isFinished() override;
    common::CommandType getCmdType() override;

   private:
    void startAction();  // does the initial play/stop/skip call
    bool onPlaybackStatus(common::PlaybackStatus s);

   private:
    services::IPlayerService& mPlayerService;
    services::IStationRepository& mStationRepo;
    common::Button mButton;
    bool mStarted{false};
    bool mFinished{false};
};

}  // namespace commands
