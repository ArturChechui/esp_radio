#include "AppController.hpp"

#include <functional>

#include "ConnectWifiCommand.hpp"
#include "Dumpers.hpp"
#include "IEventQueue.hpp"
#include "IFileSystem.hpp"
#include "IHttpClient.hpp"
#include "IInputService.hpp"
#include "IJsonParser.hpp"
#include "IPersistentStorage.hpp"
#include "IPlayerService.hpp"
#include "ISensorService.hpp"
#include "IStationRepository.hpp"
#include "IWifiService.hpp"
#include "Overloaded.hpp"
#include "PlayStopSkipCommand.hpp"
#include "SyncStationsCommand.hpp"

// IDF
#include <esp_log.h>

namespace core {
namespace {
constexpr const char* Tag = "AppController";
constexpr const char* AutoplayStorageKey = "autoplay";
constexpr uint32_t AutoplayEnabled = 1U;
constexpr uint32_t AutoplayDisabled = 0U;
}  // namespace

AppController::AppController(services::IWifiService& wifiService,
                             services::IPlayerService& playerService,
                             services::IStationRepository& stationRepository,
                             services::ISensorService& sensorService,
                             services::IInputService& inputService,
                             adapters::IHttpClient& httpClient, adapters::IFileSystem& fileSystem,
                             common::IJsonParser& jsonParser, common::IEventQueue& uiEventQueue,
                             adapters::IPersistentStorage& persistentStorage)
    : mWifiService(wifiService),
      mPlayerService(playerService),
      mStationRepository(stationRepository),
      mSensorService(sensorService),
      mInputService(inputService),
      mHttpClient(httpClient),
      mFileSystem(fileSystem),
      mJsonParser(jsonParser),
      mUiEventQueue(uiEventQueue),
      mPersistentStorage(persistentStorage),
      mCurrentCmd(nullptr),
      mInputLocked(false),
      mAutoplayState(AutoplayDisabled) {}

void AppController::onEvent(const common::AppEvent& e) {
    ESP_LOGI(Tag, "onEvent(%s)", common::dump(e).c_str());

    processCommandLane(e);
    processUiLane(e);
}

void AppController::processCommandLane(const common::AppEvent& e) {
    if (mInputLocked && isUserInputEvent(e)) {
        return;
    }

    if (!mCurrentCmd || isSimpleAction(e)) {
        std::visit(
            common::Overloaded{
                [this](const common::SystemInitedEvent&) {
                    mCurrentCmd =
                        std::make_unique<commands::ConnectWifiCommand>(mWifiService, mUiEventQueue);
                },
                [this](const common::ButtonPressedEvent& b) {
                    mCurrentCmd = std::make_unique<commands::PlayStopSkipCommand>(
                        mPlayerService, mStationRepository, mUiEventQueue, b.button);
                },
                [this](const common::ButtonLongPressedEvent& b) {
                    if (b.button == common::Button::Next) {
                        const bool val = mSensorService.toggleClapFeature();
                        (void)mUiEventQueue.post(
                            common::ClapFeatureStateChangedEvent{.isEnabled = val});
                    } else if (b.button == common::Button::PlayStop) {
                        mCurrentCmd = std::make_unique<commands::SyncStationsCommand>(
                            mPlayerService, mWifiService, mHttpClient, mFileSystem, mJsonParser,
                            mStationRepository, mUiEventQueue);
                        mInputLocked = true;
                    }
                },
                [this](const common::VolumeChangedEvent& v) { mPlayerService.setVolume(v.volume); },
                [this](const common::PlaybackStatusChangedEvent& p) {
                    const bool isPlaybackInactive = (p.status != common::PlaybackStatus::Playing &&
                                                     p.status != common::PlaybackStatus::Buffering);

                    mSensorService.startClapDetection(isPlaybackInactive);

                    const auto autoplay = (isPlaybackInactive) ? AutoplayDisabled : AutoplayEnabled;
                    if (mAutoplayState != autoplay) {
                        mAutoplayState = autoplay;
                        (void)mPersistentStorage.setU32(AutoplayStorageKey, autoplay);
                    }
                },
                [this](const common::LightLevelUpdateEvent& l) {
                    mInputService.setMode(l.lux <= services::NightLux);
                },
                [](const auto&) {}},
            e);
    }

    if (!mCurrentCmd) {
        return;
    }

    mCurrentCmd->handle(e);
    if (mCurrentCmd->isFinished()) {
        postCommandHandling();
    }
}

void AppController::processUiLane(const common::AppEvent& e) {
    std::visit(common::Overloaded{
                   [this](const common::TempHumidUpdateEvent& x) { mUiEventQueue.post(x); },
                   [this](const common::BatteryLevelUpdateEvent& x) { mUiEventQueue.post(x); },
                   [this](const common::WifiStateChangedEvent& x) { mUiEventQueue.post(x); },
                   [this](const common::PlaybackStatusChangedEvent& x) { mUiEventQueue.post(x); },
                   [this](const common::VolumeChangedEvent& x) { mUiEventQueue.post(x); },
                   [](const auto&) {}},
               e);
}

void AppController::postCommandHandling() {
    if (!mCurrentCmd) {
        return;
    }

    auto cmd = std::move(mCurrentCmd);
    mInputLocked = false;

    switch (cmd->getCmdType()) {
        case common::CommandType::ConnectWifi: {
            if (cmd->getResult().value_or(false)) {
                triggerAutoplayIfEnabled();
            }
            break;
        }
        case common::CommandType::PlayStopSkip:
        case common::CommandType::SyncStations:
        default:
            break;
    }
}

bool AppController::isUserInputEvent(const common::AppEvent& e) const {
    return std::visit(common::Overloaded{
                          [](const common::ButtonPressedEvent&) { return true; },
                          [](const common::ButtonLongPressedEvent&) { return true; },
                          [](const auto&) { return false; },
                      },
                      e);
}

bool AppController::isSimpleAction(const common::AppEvent& e) const {
    return std::visit(common::Overloaded{
                          [](const common::PlaybackStatusChangedEvent&) { return true; },
                          [](const common::VolumeChangedEvent&) { return true; },
                          [](const common::LightLevelUpdateEvent&) { return true; },
                          [](const auto&) { return false; },
                      },
                      e);
}

void AppController::triggerAutoplayIfEnabled() {
    (void)mPersistentStorage.getU32(AutoplayStorageKey, mAutoplayState);

    if (AutoplayEnabled == mAutoplayState) {
        mCurrentCmd = std::make_unique<commands::PlayStopSkipCommand>(
            mPlayerService, mStationRepository, mUiEventQueue, common::Button::PlayStop);
    }
}

}  // namespace core
