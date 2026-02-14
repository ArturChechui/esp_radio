#include "PlayStopSkipCommand.hpp"

#include <esp_log.h>

#include "IPlayerService.hpp"
#include "IStationRepository.hpp"

namespace commands {
namespace {
constexpr const char* Tag = "PlayStopSkipCmd";

static bool isPlayingOrBuffering(common::PlaybackStatus s) {
    return (s == common::PlaybackStatus::Buffering) || (s == common::PlaybackStatus::Playing);
}
}  // namespace

PlayStopSkipCommand::PlayStopSkipCommand(services::IPlayerService& playerService,
                                         services::IStationRepository& stationRepo,
                                         const common::Button btn)
    : mPlayerService(playerService),
      mStationRepo(stationRepo),
      mButton(btn),
      mStarted(false),
      mFinished(false) {}

bool PlayStopSkipCommand::handle(const common::AppEvent& e) {
    if (mFinished) {
        return false;
    }

    // First call that triggers the action
    if (!mStarted) {
        mStarted = true;
        startAction();
        // command continues until we observe status change
        return true;
    }

    // TODO: use Overloaded here as well?
    // Finish conditions
    if (std::holds_alternative<common::PlaybackStatusChangedEvent>(e)) {
        const auto ev = std::get<common::PlaybackStatusChangedEvent>(e);
        return onPlaybackStatus(ev.status);  // returns handled
    }

    // Ignore other events while in-flight (or handle extra button presses if you want)
    return false;
}

void PlayStopSkipCommand::startAction() {
    const auto st = mPlayerService.getStatus();

    switch (mButton) {
        case common::Button::PlayStop: {
            if (isPlayingOrBuffering(st)) {
                ESP_LOGI(Tag, "Toggle -> stop");
                (void)mPlayerService.stop();
            } else {
                const auto& station = mStationRepo.currentStation();
                ESP_LOGI(Tag, "Toggle -> play %s", station.name.c_str());
                (void)mPlayerService.playStation(station.url);
            }
            break;
        }
        case common::Button::Up: {
            if (isPlayingOrBuffering(st)) {
                ESP_LOGI(Tag, "Skip -> stop");
                (void)mPlayerService.stop();
            }

            const auto& station = mStationRepo.nextStation();
            ESP_LOGI(Tag, "Skip -> play %s", station.name.c_str());
            (void)mPlayerService.playStation(station.url);
            break;
        }
        case common::Button::Down: {
            if (isPlayingOrBuffering(st)) {
                ESP_LOGI(Tag, "Skip -> stop");
                (void)mPlayerService.stop();
            }

            const auto& station = mStationRepo.prevStation();
            ESP_LOGI(Tag, "Skip -> play %s", station.name.c_str());
            (void)mPlayerService.playStation(station.url);
            break;
        }
    }
}

bool PlayStopSkipCommand::onPlaybackStatus(common::PlaybackStatus s) {
    // finish on error to avoid "stuck" command
    if (s == common::PlaybackStatus::Error) {
        ESP_LOGW(Tag, "Finished on Error");
        mFinished = true;
        return true;
    }

    switch (mButton) {
        case common::Button::PlayStop: {
            // TODO: We can’t know if user intended play or stop after toggle for now,
            // so accept either "started" or "stopped" end states.
            // but we know what we requested, we should save what we sent and expect result
            if (s == common::PlaybackStatus::Playing || s == common::PlaybackStatus::Buffering ||
                s == common::PlaybackStatus::Stopped) {
                mFinished = true;
                ESP_LOGI(Tag, "Finished");
            }
            break;
        }
        case common::Button::Up:
        case common::Button::Down: {
            if (s == common::PlaybackStatus::Playing || s == common::PlaybackStatus::Buffering) {
                mFinished = true;
                ESP_LOGI(Tag, "Finished");
            }
            break;
        }
    }

    return true;  // handled this status event
}

bool PlayStopSkipCommand::isFinished() {
    return mFinished;
}

}  // namespace commands
