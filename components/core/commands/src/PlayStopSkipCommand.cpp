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
      mFinished(false) {}

void PlayStopSkipCommand::handle(const common::AppEvent& e) {
    if (mFinished) {
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

void PlayStopSkipCommand::startAction() {
    const auto st = mPlayerService.getStatus();

    switch (mButton) {
        case common::Button::PlayStop: {
            if (isPlayingOrBuffering(st)) {
                ESP_LOGI(Tag, "Toggle -> stop");
                (void)mPlayerService.stop();
                mRequestedAction = Action::Stop;
            } else {
                const auto& station = mStationRepo.currentStation();
                ESP_LOGI(Tag, "Toggle -> play %s", station.name.c_str());
                (void)mPlayerService.playStation(station.url);
                mRequestedAction = Action::Play;
            }
            break;
        }
        case common::Button::Next: {
            if (isPlayingOrBuffering(st)) {
                ESP_LOGI(Tag, "Skip -> stop");
                (void)mPlayerService.stop();
            }

            const auto& station = mStationRepo.nextStation();
            ESP_LOGI(Tag, "Skip -> play %s", station.name.c_str());
            (void)mPlayerService.playStation(station.url);
            mUiEventQueue.post(common::CurrentStationChangedEvent{});
            mRequestedAction = Action::SkipNext;
            break;
        }
        case common::Button::Previous: {
            if (isPlayingOrBuffering(st)) {
                ESP_LOGI(Tag, "Skip -> stop");
                (void)mPlayerService.stop();
            }

            const auto& station = mStationRepo.prevStation();
            ESP_LOGI(Tag, "Skip -> play %s", station.name.c_str());
            (void)mPlayerService.playStation(station.url);
            mUiEventQueue.post(common::CurrentStationChangedEvent{});
            mRequestedAction = Action::SkipPrevious;
            break;
        }
    }
}

void PlayStopSkipCommand::onPlaybackStatus(common::PlaybackStatus s) {
    // finish on error to avoid "stuck" command
    if (s == common::PlaybackStatus::Error) {
        ESP_LOGW(Tag, "Finished on Error");
        mFinished = true;
        return;
    }

    switch (mRequestedAction) {
        case Action::SkipNext:
        case Action::SkipPrevious:
        case Action::Play: {
            if (s == common::PlaybackStatus::Playing || s == common::PlaybackStatus::Buffering) {
                mFinished = true;
                ESP_LOGI(Tag, "Finished");
            }
            break;
        }
        case Action::Stop: {
            if (s == common::PlaybackStatus::Stopped) {
                mFinished = true;
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

bool PlayStopSkipCommand::isFinished() {
    return mFinished;
}

}  // namespace core::commands
