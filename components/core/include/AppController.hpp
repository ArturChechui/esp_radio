#pragma once

#include <memory>

#include "Events.hpp"
#include "ICommand.hpp"
#include "IEventHandler.hpp"

namespace common {
class IEventQueue;
class IManifestParser;
class IJsonParser;
}  // namespace common

namespace adapters {
class IHttpClient;
class IFileSystem;
}  // namespace adapters

namespace services {
class IPlayerService;
class IStationRepository;
class IWifiService;
class ISensorService;
class IInputService;
}  // namespace services

namespace core {
namespace commands {
class ICommand;
}  // namespace commands

class IUiEventSink;

class AppController : public common::IEventHandler {
   public:
    AppController(services::IWifiService& wifiService, services::IPlayerService& playerService,
                  services::IStationRepository& stationRepository,
                  services::ISensorService& sensorService, services::IInputService& inputService,
                  adapters::IHttpClient& httpClient, adapters::IFileSystem& fileSystem,
                  common::IJsonParser& jsonParser, common::IEventQueue& uiEventQueue);
    ~AppController() override = default;

    void onEvent(const common::AppEvent& e) override;

   private:
    void processCommandLane(const common::AppEvent& e);
    void processUiLane(const common::AppEvent& e);
    bool isUserInputEvent(const common::AppEvent& e) const;
    bool isSimpleAction(const common::AppEvent& e) const;

    services::IWifiService& mWifiService;
    services::IPlayerService& mPlayerService;
    services::IStationRepository& mStationRepository;
    services::ISensorService& mSensorService;
    services::IInputService& mInputService;
    adapters::IHttpClient& mHttpClient;
    adapters::IFileSystem& mFileSystem;
    common::IJsonParser& mJsonParser;
    common::IEventQueue& mUiEventQueue;

    std::unique_ptr<commands::ICommand> mCurrentCmd;
    bool mInputLocked;
};

}  // namespace core
