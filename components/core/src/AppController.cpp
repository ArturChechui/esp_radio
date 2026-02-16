#include "AppController.hpp"

#include "Dumpers.hpp"
#include "IEventQueue.hpp"
#include "IPlayerService.hpp"
#include "IStationRepository.hpp"
#include "Overloaded.hpp"
#include "PlayStopSkipCommand.hpp"

// IDF
#include <esp_log.h>

namespace core {
namespace {
constexpr const char* Tag = "AppController";
}  // namespace

AppController::AppController(services::IPlayerService& playerService,
                             services::IStationRepository& stationRepository,
                             common::IEventQueue& uiEventQueue)
    : mPlayerService(playerService),
      mStationRepository(stationRepository),
      mUiEventQueue(uiEventQueue),
      mCurrentCmd(nullptr) {}

bool AppController::init() {
    ESP_LOGI(Tag, "Initializing AppController");

    return true;
}

void AppController::onEvent(const common::AppEvent& event) {
    ESP_LOGI(Tag, "onEvent(%s)", common::dump(event).c_str());

    std::visit(
        common::Overloaded{
            [this](const common::SystemReadyEvent&) {
                // TODO: Start restore lastmode cmd
            },
            [this](const common::ButtonPressedEvent& b) {
                mCurrentCmd = std::make_unique<commands::PlayStopSkipCommand>(
                    mPlayerService, mStationRepository, b.button);
                if (mCurrentCmd && mCurrentCmd->handle(b) && mCurrentCmd->isFinished()) {
                    mCurrentCmd.reset();
                }

                if (b.button == common::Button::Previous || b.button == common::Button::Next) {
                    mUiEventQueue.post(common::CurrentStationChangedEvent{});
                }
            },
            [this](const common::TempHumidUpdateEvent& t) { mUiEventQueue.post(t); },
            [this](const common::WifiStateChangedEvent& w) { mUiEventQueue.post(w); },
            [this](const common::PlaybackStatusChangedEvent& p) {
                if (mCurrentCmd && !mCurrentCmd->isFinished()) {
                    if (mCurrentCmd->handle(p) && mCurrentCmd->isFinished()) {
                        mCurrentCmd.reset();
                    }
                }
                mUiEventQueue.post(p);
            },
            [this](const common::VolumeChangedEvent& v) {
                mPlayerService.setVolume(v.volume);
                mUiEventQueue.post(v);
            },
            [](const auto&) {
                // ignore other events
            }},
        event);
}

}  // namespace core
