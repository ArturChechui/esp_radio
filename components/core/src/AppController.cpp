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

    // TODO: any initialization logic

    return true;
}

void AppController::onEvent(const common::AppEvent& event) {
    ESP_LOGI(Tag, "onEvent(%s)", common::dump(event).c_str());

    // try to handle it by the current cmd
    if (mCurrentCmd && !mCurrentCmd->isFinished()) {
        if (mCurrentCmd->handle(event)) {
            // handled
            if (mCurrentCmd->isFinished()) {
                // command is finished, so can be destroyed
                mCurrentCmd.reset();
            }
            // TODO: temporary comment since so far it is not clear when we should intercept the
            // event and ignore it further
            // return;
        }
    }

    std::visit(common::Overloaded{
                   [this](const common::SystemReadyEvent&) {
                       // TODO: Start restore lastmode cmd
                   },
                   [this](const common::ButtonPressedEvent& b) {
                       // TODO: handle the case when play is active and stop is being pressed or
                       // vice versa
                       mCurrentCmd = std::make_unique<commands::PlayStopSkipCommand>(
                           mPlayerService, mStationRepository, b.button);
                       if (mCurrentCmd && mCurrentCmd->handle(b) && mCurrentCmd->isFinished()) {
                           mCurrentCmd.reset();
                       }

                       if (b.button == common::Button::Down || b.button == common::Button::Up) {
                           mUiEventQueue.post(common::CurrentStationChangedEvent{});
                       }
                   },
                   [this](const common::TempHumidUpdateEvent& t) { mUiEventQueue.post(t); },
                   [this](const common::WifiStateChangedEvent& w) { mUiEventQueue.post(w); },
                   [this](const common::PlaybackStatusChangedEvent& p) { mUiEventQueue.post(p); },
                   [](const auto&) {
                       // ignore other events
                   }},
               event);
}

}  // namespace core
