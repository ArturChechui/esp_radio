#include "PlayStopSkipCommand.hpp"

#include <esp_log.h>

#include "IEventQueue.hpp"
#include "IPlayerService.hpp"
#include "IStationRepository.hpp"
#include "Overloaded.hpp"

namespace core::commands {
namespace {
constexpr const char* Tag = "PlayStopSkipCmd";

static bool isPlayingOrBuffering(common::PlaybackStatus s) {
    return (s == common::PlaybackStatus::Buffering) || (s == common::PlaybackStatus::Playing);
}
}  // namespace

PlayStopSkipCommand::PlayStopSkipCommand(services::IPlayerService& playerService,
                                         services::IStationRepository& stationRepo,
                                         common::IEventQueue& uiEventQueue,
                                         const common::Button btn)
    : mPlayerService(playerService),
      mStationRepo(stationRepo),
      mUiEventQueue(uiEventQueue),
      mButton(btn),
      mRequestedAction(Action::Play),
      mStarted(false),
      mResult(std::nullopt) {}

void PlayStopSkipCommand::handle(const common::AppEvent& e) {
    if (mResult.has_value()) {
        return;
    }

    if (!mStarted) {
        mStarted = true;
        startAction();
        return;
    }

    std::visit(common::Overloaded{[this](const common::PlaybackStatusChangedEvent& p) {
                                      onPlaybackStatus(p.status);
                                  },
                                  [](const auto&) {}},
               e);
}

bool PlayStopSkipCommand::isFinished() {
    return mResult.has_value();
}

common::CommandType PlayStopSkipCommand::getCmdType() {
    return common::CommandType::PlayStopSkip;
}

std::optional<bool> PlayStopSkipCommand::getResult() {
    return mResult;
}

void PlayStopSkipCommand::startAction() {
    const auto st = mPlayerService.getStatus();

    if (mButton == common::Button::PlayStop) {
        if (isPlayingOrBuffering(st)) {
            ESP_LOGI(Tag, "Stopping playback");
            (void)mPlayerService.stop();
            mRequestedAction = Action::Stop;
        } else {
            const auto& station = mStationRepo.currentStation();
            ESP_LOGI(Tag, "Playing %s", station.name.c_str());
            (void)mPlayerService.playStation(station.url);
            mRequestedAction = Action::Play;
        }
    } else {
        if (isPlayingOrBuffering(st)) {
            ESP_LOGI(Tag, "Stopping before skip");
            (void)mPlayerService.stop();
        }

        const auto& station = (mButton == common::Button::Next) ? mStationRepo.nextStation()
                                                                : mStationRepo.prevStation();
        ESP_LOGI(Tag, "Station after skip: %s", station.name.c_str());
        mUiEventQueue.post(common::CurrentStationChangedEvent{});

        mResult = true;
        ESP_LOGI(Tag, "Finished");
    }
}

void PlayStopSkipCommand::onPlaybackStatus(common::PlaybackStatus s) {
    // finish on error to avoid "stuck" command
    if (s == common::PlaybackStatus::Error) {
        ESP_LOGW(Tag, "Finished with Error");
        mResult = false;
        return;
    }

    switch (mRequestedAction) {
        case Action::Play: {
            if (s == common::PlaybackStatus::Playing || s == common::PlaybackStatus::Buffering) {
                mResult = true;
                ESP_LOGI(Tag, "Finished");
            }
            break;
        }
        case Action::Stop: {
            if (s == common::PlaybackStatus::Stopped) {
                mResult = true;
                ESP_LOGI(Tag, "Finished");
            }
            break;
        }
        default: {
            break;
        }
    }

    return;  // handled this status event
}

}  // namespace core::commands
